#include "providers/ollama_provider.h"

#include "core/json_utils.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkReply>
#include <QUrl>

#include <utility>

namespace {
const QString kGeneratePath = QStringLiteral("/api/generate");
const QString kVersionPath = QStringLiteral("/api/version");
const QString kTagsPath = QStringLiteral("/api/tags");

QString withoutTrailingSlashes(QString url)
{
    while (url.endsWith(QLatin1Char('/'))) url.chop(1);
    return url;
}

int clampTimeout(int ms)
{
    return ms < 1 ? 1 : ms;
}
}

OllamaProvider::OllamaProvider(QString baseUrl, int timeoutMs, QObject *parent)
    : QObject(parent),
      m_baseUrl(withoutTrailingSlashes(std::move(baseUrl))),
      m_timeoutMs(clampTimeout(timeoutMs))
{
}

void OllamaProvider::analyze(const AIRequest &request, std::function<void(AIResponse)> callback)
{
    if (request.model.trimmed().isEmpty()) {
        callback({false, {}, QStringLiteral("Не указана модель для анализа.")});
        return;
    }

    QNetworkRequest networkRequest = buildRequest(kGeneratePath);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    trackReply(m_network.post(networkRequest, buildRequestBody(request)), [callback](QNetworkReply *finishedReply) {
        const QByteArray data = finishedReply->readAll();
        const int statusCode = finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        AIResponse response;
        if (statusCode < 100) {
            response.success = false;
            response.errorMessage = QStringLiteral("Ollama недоступна: %1").arg(finishedReply->errorString());
        } else if (statusCode < 200 || statusCode >= 300) {
            response.success = false;
            response.errorMessage = QStringLiteral("Ollama вернула HTTP %1: %2")
                .arg(statusCode)
                .arg(parseOllamaError(data));
        } else {
            response.rawText = parseResponseText(data);
            if (response.rawText.isEmpty()) {
                response.success = false;
                response.errorMessage = QStringLiteral("Ollama вернула пустой или некорректный ответ.");
            } else {
                response.success = true;
            }
        }
        finishedReply->deleteLater();
        callback(std::move(response));
    });
}

QString OllamaProvider::providerName() const { return QStringLiteral("Ollama"); }

void OllamaProvider::checkAvailability(std::function<void(bool available, const QString &errorMessage)> callback)
{
    trackReply(m_network.get(buildRequest(kVersionPath)), [callback](QNetworkReply *finishedReply) {
        finishedReply->readAll();
        const bool available = isSuccessfulReply(finishedReply);
        const QString error = available ? QString() : finishedReply->errorString();
        finishedReply->deleteLater();
        callback(available, error);
    });
}

void OllamaProvider::fetchAvailableModels(std::function<void(QStringList models, const QString &errorMessage)> callback)
{
    trackReply(m_network.get(buildRequest(kTagsPath)), [callback](QNetworkReply *finishedReply) {
        const QByteArray data = finishedReply->readAll();
        QStringList models;
        QString error;
        if (!isSuccessfulReply(finishedReply)) {
            error = finishedReply->errorString();
        } else {
            QJsonObject object;
            if (JsonUtils::parseObject(data, object)) {
                const QJsonArray names = object.value(QStringLiteral("models")).toArray();
                for (const QJsonValue &value : names) {
                    if (value.isObject()) {
                        const QString name = value.toObject().value(QStringLiteral("name")).toString();
                        if (!name.isEmpty()) models.append(name);
                    }
                }
            } else {
                error = QStringLiteral("Ollama вернула некорректный ответ /api/tags.");
            }
        }
        finishedReply->deleteLater();
        callback(models, error);
    });
}

QString OllamaProvider::baseUrl() const { return m_baseUrl; }

void OllamaProvider::setBaseUrl(const QString &url)
{
    m_baseUrl = withoutTrailingSlashes(url);
}

int OllamaProvider::timeoutMs() const { return m_timeoutMs; }

void OllamaProvider::setTimeoutMs(int ms)
{
    m_timeoutMs = clampTimeout(ms);
}

QNetworkRequest OllamaProvider::buildRequest(const QString &path) const
{
    QNetworkRequest request(QUrl(m_baseUrl + path));
    request.setTransferTimeout(m_timeoutMs);
    return request;
}

void OllamaProvider::trackReply(QNetworkReply *reply, std::function<void(QNetworkReply *)> handler)
{
    m_pending.insert(reply, std::move(handler));
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReplyFinished(reply); });
}

bool OllamaProvider::isSuccessfulReply(QNetworkReply *reply)
{
    return reply->error() == QNetworkReply::NoError
           && reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
}

QByteArray OllamaProvider::buildRequestBody(const AIRequest &request)
{
    QJsonObject options;
    options.insert(QStringLiteral("temperature"), request.temperature);

    QJsonObject object;
    object.insert(QStringLiteral("model"), request.model);
    if (!request.systemPrompt.isEmpty()) object.insert(QStringLiteral("system"), request.systemPrompt);
    object.insert(QStringLiteral("prompt"), request.userPrompt);
    object.insert(QStringLiteral("stream"), false);
    object.insert(QStringLiteral("format"), QStringLiteral("json"));
    object.insert(QStringLiteral("options"), options);

    return JsonUtils::toCompactJson(object);
}

QString OllamaProvider::parseResponseText(const QByteArray &body)
{
    QJsonObject object;
    if (!JsonUtils::parseObject(body, object)) return {};
    const QJsonValue response = object.value(QStringLiteral("response"));
    if (!response.isString()) return {};
    return response.toString().trimmed();
}

QString OllamaProvider::parseOllamaError(const QByteArray &body)
{
    QJsonObject object;
    if (JsonUtils::parseObject(body, object)) {
        const QJsonValue error = object.value(QStringLiteral("error"));
        if (error.isString()) return error.toString();
    }
    return QString::fromUtf8(body).trimmed();
}

void OllamaProvider::handleReplyFinished(QNetworkReply *reply)
{
    const auto callbackIt = m_pending.find(reply);
    if (callbackIt == m_pending.end()) return;
    const std::function<void(QNetworkReply *)> callback = callbackIt.value();
    m_pending.erase(callbackIt);
    callback(reply);
}

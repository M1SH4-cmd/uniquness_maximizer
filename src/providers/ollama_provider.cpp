#include "providers/ollama_provider.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <utility>

namespace {
const QString kGeneratePath = QStringLiteral("/api/generate");
const QString kVersionPath = QStringLiteral("/api/version");
const QString kTagsPath = QStringLiteral("/api/tags");
}

OllamaProvider::OllamaProvider(QString baseUrl, int timeoutMs, QObject *parent)
    : QObject(parent),
      m_baseUrl(std::move(baseUrl)),
      m_timeoutMs(timeoutMs)
{
    while (m_baseUrl.endsWith(QLatin1Char('/'))) m_baseUrl.chop(1);
    if (m_timeoutMs < 1) m_timeoutMs = 1;
}

OllamaProvider::~OllamaProvider()
{
    const QHash<QNetworkReply *, std::function<void(QNetworkReply *)>> pending = m_pending;
    m_pending.clear();
    for (auto it = pending.cbegin(); it != pending.cend(); ++it) {
        QNetworkReply *reply = it.key();
        reply->disconnect(this);
        reply->abort();
        it.value()(reply);
    }
}

void OllamaProvider::analyze(const AIRequest &request, std::function<void(AIResponse)> callback)
{
    if (request.model.trimmed().isEmpty()) {
        callback({false, {}, QStringLiteral("Не указана модель для анализа.")});
        return;
    }

    const QByteArray body = buildRequestBody(request);
    QNetworkRequest networkRequest(QUrl(m_baseUrl + kGeneratePath));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    networkRequest.setTransferTimeout(m_timeoutMs);

    QNetworkReply *reply = m_network.post(networkRequest, body);
    m_pending.insert(reply, [callback](QNetworkReply *finishedReply) {
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
            QString parseError;
            response.rawText = parseResponseText(data, &parseError);
            if (response.rawText.isEmpty()) {
                response.success = false;
                response.errorMessage = QStringLiteral("Ollama вернула пустой или некорректный ответ: %1")
                    .arg(parseError.isEmpty() ? QStringLiteral("поле 'response' пусто") : parseError);
            } else {
                response.success = true;
            }
        }
        finishedReply->deleteLater();
        callback(std::move(response));
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReplyFinished(reply); });
}

QString OllamaProvider::providerName() const { return QStringLiteral("Ollama"); }

void OllamaProvider::checkAvailability(std::function<void(bool available, const QString &errorMessage)> callback)
{
    QNetworkRequest networkRequest(QUrl(m_baseUrl + kVersionPath));
    networkRequest.setTransferTimeout(m_timeoutMs);

    QNetworkReply *reply = m_network.get(networkRequest);
    m_pending.insert(reply, [callback](QNetworkReply *finishedReply) {
        const QByteArray data = finishedReply->readAll();
        const int statusCode = finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        const bool available = finishedReply->error() == QNetworkReply::NoError && statusCode == 200;
        QString error;
        if (!available) {
            if (statusCode < 100) {
                error = QStringLiteral("Ollama недоступна: %1").arg(finishedReply->errorString());
            } else {
                error = QStringLiteral("Ollama вернула HTTP %1: %2").arg(statusCode).arg(parseOllamaError(data));
            }
        }
        finishedReply->deleteLater();
        callback(available, error);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReplyFinished(reply); });
}

void OllamaProvider::fetchAvailableModels(std::function<void(QStringList models, const QString &errorMessage)> callback)
{
    QNetworkRequest networkRequest(QUrl(m_baseUrl + kTagsPath));
    networkRequest.setTransferTimeout(m_timeoutMs);

    QNetworkReply *reply = m_network.get(networkRequest);
    m_pending.insert(reply, [callback](QNetworkReply *finishedReply) {
        const QByteArray data = finishedReply->readAll();
        QStringList models;
        QString error;
        const bool ok = finishedReply->error() == QNetworkReply::NoError
                        && finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() == 200;
        if (ok) {
            QJsonParseError parseError;
            const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                error = QStringLiteral("Ollama вернула некорректный ответ /api/tags: %1.").arg(parseError.errorString());
            } else {
                const QJsonArray names = document.object().value(QStringLiteral("models")).toArray();
                for (const QJsonValue &value : names) {
                    if (value.isObject()) {
                        const QString name = value.toObject().value(QStringLiteral("name")).toString();
                        if (!name.isEmpty()) models.append(name);
                    }
                }
            }
        } else if (finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() < 100) {
            error = QStringLiteral("Ollama недоступна: %1").arg(finishedReply->errorString());
        } else {
            error = QStringLiteral("Ollama вернула HTTP %1: %2")
                .arg(finishedReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt())
                .arg(parseOllamaError(data));
        }
        finishedReply->deleteLater();
        callback(models, error);
    });
    connect(reply, &QNetworkReply::finished, this, [this, reply]() { handleReplyFinished(reply); });
}

QString OllamaProvider::baseUrl() const { return m_baseUrl; }

void OllamaProvider::setBaseUrl(const QString &url)
{
    m_baseUrl = url;
    while (m_baseUrl.endsWith(QLatin1Char('/'))) m_baseUrl.chop(1);
}

int OllamaProvider::timeoutMs() const { return m_timeoutMs; }

void OllamaProvider::setTimeoutMs(int ms)
{
    if (ms < 1) ms = 1;
    m_timeoutMs = ms;
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

    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

QString OllamaProvider::parseResponseText(const QByteArray &body, QString *errorMessage)
{
    const auto fail = [errorMessage](const QString &message) {
        if (errorMessage) *errorMessage = message;
        return QString();
    };

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        return fail(QStringLiteral("тело ответа не является JSON (%1)").arg(parseError.errorString()));
    }
    if (!document.isObject()) return fail(QStringLiteral("тело ответа не является JSON-объектом"));
    const QJsonValue response = document.object().value(QStringLiteral("response"));
    if (!response.isString()) return fail(QStringLiteral("в ответе нет строкового поля 'response'"));
    if (errorMessage) errorMessage->clear();
    return response.toString().trimmed();
}

QString OllamaProvider::parseOllamaError(const QByteArray &body)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && document.isObject()) {
        const QJsonValue error = document.object().value(QStringLiteral("error"));
        if (error.isString()) return error.toString();
    }
    return QString::fromUtf8(body).trimmed();
}

void OllamaProvider::handleReplyFinished(QNetworkReply *reply)
{
    const auto callbackIt = m_pending.find(reply);
    if (callbackIt == m_pending.end()) {
        reply->deleteLater();
        return;
    }
    const std::function<void(QNetworkReply *)> callback = callbackIt.value();
    m_pending.erase(callbackIt);
    callback(reply);
}

void OllamaProvider::cancelAll()
{
    for (auto it = m_pending.begin(); it != m_pending.end(); ) {
        QNetworkReply *reply = it.key();
        ++it; // Increment before potential removal to avoid iterator invalidation
        reply->abort();
    }
}

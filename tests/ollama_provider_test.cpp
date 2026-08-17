#include "providers/ollama_provider.h"

#include "async_test_support.h"
#include "test_support.h"

#include <QHostAddress>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QTcpServer>
#include <QTcpSocket>

#include <QtMath>

namespace {
using TestSupport::require;
using TestSupport::runAnalyze;

class FakeOllamaServer final : public QObject
{
public:
    FakeOllamaServer(int statusCode, const QByteArray &body, QObject *parent = nullptr)
        : QObject(parent), m_statusCode(statusCode), m_body(body)
    {
    }

    bool start()
    {
        connect(&m_server, &QTcpServer::newConnection, this, &FakeOllamaServer::onNewConnection);
        return m_server.listen(QHostAddress::LocalHost, 0);
    }

    quint16 port() const { return m_server.serverPort(); }

private:
    void onNewConnection()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *socket = m_server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                socket->readAll();
                if (m_responded) return;
                m_responded = true;
                const QString reason = m_statusCode == 200 ? QStringLiteral("OK") : QStringLiteral("Error");
                const QByteArray header = "HTTP/1.1 " + QByteArray::number(m_statusCode) + " " + reason.toUtf8() + "\r\n"
                                          "Content-Type: application/json\r\n"
                                          "Content-Length: " + QByteArray::number(m_body.size()) + "\r\n"
                                          "Connection: close\r\n\r\n";
                socket->write(header + m_body);
                socket->flush();
            });
            connect(socket, &QTcpSocket::disconnected, socket, &QObject::deleteLater);
        }
    }

    QTcpServer m_server;
    int m_statusCode;
    QByteArray m_body;
    bool m_responded = false;
};

bool testRequestBodyBuilding()
{
    const AIRequest request{QStringLiteral("Системный промпт"),
                            QStringLiteral("Проанализируй текст."),
                            QStringLiteral("qwen2.5:3b"),
                            0.42};
    const QJsonDocument document = QJsonDocument::fromJson(OllamaProvider::buildRequestBody(request));
    if (!require(document.isObject(), QStringLiteral("request: тело не является JSON"))) return false;
    const QJsonObject object = document.object();
    return require(object.value(QStringLiteral("model")).toString() == QStringLiteral("qwen2.5:3b"), QStringLiteral("request: model неверна"))
           && require(object.value(QStringLiteral("system")).toString() == QStringLiteral("Системный промпт"), QStringLiteral("request: system неверен"))
           && require(object.value(QStringLiteral("prompt")).toString() == QStringLiteral("Проанализируй текст."), QStringLiteral("request: prompt неверен"))
           && require(object.value(QStringLiteral("stream")).toBool(true) == false, QStringLiteral("request: stream должен быть false"))
           && require(object.value(QStringLiteral("format")).toString() == QStringLiteral("json"), QStringLiteral("request: json mode не включён"))
           && require(qFuzzyCompare(object.value(QStringLiteral("options")).toObject().value(QStringLiteral("temperature")).toDouble(), 0.42),
                      QStringLiteral("request: temperature неверна"));
}

bool testSuccessfulResponse()
{
    FakeOllamaServer server(200, R"({"model":"qwen2.5:3b","done":true,"response":"{\"issues\":[]}"})");
    if (!require(server.start(), QStringLiteral("success: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const AIResponse response = runAnalyze(provider, {QStringLiteral("sys"), QStringLiteral("prompt"), QStringLiteral("qwen2.5:3b"), 0.2});
    return require(response.success, QStringLiteral("success: ответ не отмечен успешным"))
           && require(response.rawText == QStringLiteral("{\"issues\":[]}"), QStringLiteral("success: rawText извлечён неверно"))
           && require(response.errorMessage.isEmpty(), QStringLiteral("success: неожиданное сообщение об ошибке"));
}

bool testOllamaUnavailable()
{
    QTcpServer probe;
    if (!require(probe.listen(QHostAddress::LocalHost, 0), QStringLiteral("unavailable: не удалось получить порт"))) return false;
    const quint16 closedPort = probe.serverPort();
    probe.close();
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(closedPort), 1500);
    const AIResponse response = runAnalyze(provider, {QStringLiteral("sys"), QStringLiteral("prompt"), QStringLiteral("model"), 0.2});
    return require(!response.success, QStringLiteral("unavailable: соединение не распознано как ошибка"))
           && require(!response.errorMessage.isEmpty(), QStringLiteral("unavailable: сообщение об ошибке пустое"))
           && require(response.rawText.isEmpty(), QStringLiteral("unavailable: rawText должен быть пустым"));
}

bool testMalformedResponse()
{
    FakeOllamaServer server(200, QByteArrayLiteral("definitely not json"));
    if (!require(server.start(), QStringLiteral("malformed: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const AIResponse response = runAnalyze(provider, {QStringLiteral("sys"), QStringLiteral("prompt"), QStringLiteral("model"), 0.2});
    return require(!response.success, QStringLiteral("malformed: некорректный ответ принят"))
           && require(response.rawText.isEmpty(), QStringLiteral("malformed: rawText не пуст"))
           && require(!response.errorMessage.isEmpty(), QStringLiteral("malformed: сообщение об ошибке пустое"));
}

bool testModelNotFound()
{
    FakeOllamaServer server(404, R"({"error":"model 'qwen2.5:3b' not found"})");
    if (!require(server.start(), QStringLiteral("404: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const AIResponse response = runAnalyze(provider, {QStringLiteral("sys"), QStringLiteral("prompt"), QStringLiteral("qwen2.5:3b"), 0.2});
    return require(!response.success, QStringLiteral("404: ошибка HTTP принята как успех"))
           && require(response.errorMessage.contains(QStringLiteral("not found")), QStringLiteral("404: сообщение не описывает отсутствие модели"));
}

bool testEmptyModelRejected()
{
    OllamaProvider provider;
    const AIResponse response = runAnalyze(provider, {QStringLiteral("sys"), QStringLiteral("prompt"), QString(), 0.2});
    return require(!response.success, QStringLiteral("empty model: запрос без модели принят"))
           && require(response.errorMessage.contains(QStringLiteral("модель")), QStringLiteral("empty model: сообщение не понятное"));
}
}

TEST_SUPPORT_MAIN(testRequestBodyBuilding, testSuccessfulResponse, testOllamaUnavailable, testMalformedResponse,
                  testModelNotFound, testEmptyModelRejected)

#include "providers/ollama_provider.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTextStream>
#include <QTimer>

namespace {
bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

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
    const QByteArray &lastRequest() const { return m_lastRequest; }

private:
    void onNewConnection()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *socket = m_server.nextPendingConnection();
            connect(socket, &QTcpSocket::readyRead, this, [this, socket]() {
                m_lastRequest = socket->readAll();
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
    QByteArray m_lastRequest;
};

quint16 closedPort()
{
    QTcpServer probe;
    if (!probe.listen(QHostAddress::LocalHost, 0)) return 0;
    const quint16 port = probe.serverPort();
    probe.close();
    return port;
}

struct AvailabilityResult
{
    bool called = false;
    bool available = false;
    QString errorMessage;
};

AvailabilityResult runCheckAvailability(OllamaProvider &provider, int timeoutMs = 15000)
{
    QEventLoop loop;
    AvailabilityResult result;
    provider.checkAvailability([&](bool available, const QString &errorMessage) {
        result = {true, available, errorMessage};
        loop.quit();
    });
    if (!result.called) {
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return result;
}

struct ModelsResult
{
    bool called = false;
    QStringList models;
    QString errorMessage;
};

ModelsResult runFetchModels(OllamaProvider &provider, int timeoutMs = 15000)
{
    QEventLoop loop;
    ModelsResult result;
    provider.fetchAvailableModels([&](QStringList models, const QString &errorMessage) {
        result = {true, std::move(models), errorMessage};
        loop.quit();
    });
    if (!result.called) {
        QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
        loop.exec();
    }
    return result;
}

bool testProviderNameAndDefaults()
{
    const OllamaProvider provider;
    return require(provider.providerName() == QStringLiteral("Ollama"), QStringLiteral("defaults: имя провайдера неверно"))
           && require(provider.baseUrl() == QStringLiteral("http://127.0.0.1:11434"), QStringLiteral("defaults: базовый URL по умолчанию неверен"))
           && require(provider.timeoutMs() == 120000, QStringLiteral("defaults: таймаут по умолчанию неверен"));
}

bool testBaseUrlNormalisation()
{
    OllamaProvider provider(QStringLiteral("http://localhost:11434///"));
    if (!require(provider.baseUrl() == QStringLiteral("http://localhost:11434"), QStringLiteral("url: конструктор не убрал завершающие '/'"))) return false;
    provider.setBaseUrl(QStringLiteral("http://example.test:1234//"));
    return require(provider.baseUrl() == QStringLiteral("http://example.test:1234"), QStringLiteral("url: setBaseUrl не убрал завершающие '/'"));
}

bool testTimeoutClamping()
{
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:11434"), 0);
    if (!require(provider.timeoutMs() == 1, QStringLiteral("timeout: конструктор не ограничил неположительный таймаут"))) return false;
    provider.setTimeoutMs(-5);
    if (!require(provider.timeoutMs() == 1, QStringLiteral("timeout: setTimeoutMs не ограничил отрицательное значение"))) return false;
    provider.setTimeoutMs(2500);
    return require(provider.timeoutMs() == 2500, QStringLiteral("timeout: корректное значение не сохранено"));
}

bool testAvailabilityWhenServerAnswers()
{
    FakeOllamaServer server(200, R"({"version":"0.5.7"})");
    if (!require(server.start(), QStringLiteral("availability: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const AvailabilityResult result = runCheckAvailability(provider);
    return require(result.called, QStringLiteral("availability: callback не вызван"))
           && require(result.available, QStringLiteral("availability: доступная Ollama отмечена недоступной"))
           && require(result.errorMessage.isEmpty(), QStringLiteral("availability: неожиданное сообщение об ошибке"))
           && require(server.lastRequest().contains(QByteArrayLiteral("/api/version")), QStringLiteral("availability: запрошен неверный endpoint"));
}

bool testAvailabilityWhenServerFails()
{
    FakeOllamaServer server(500, R"({"error":"internal"})");
    if (!require(server.start(), QStringLiteral("availability error: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const AvailabilityResult httpError = runCheckAvailability(provider);
    if (!require(httpError.called && !httpError.available && !httpError.errorMessage.isEmpty(),
                 QStringLiteral("availability error: HTTP 500 принят как доступность"))) return false;

    const quint16 port = closedPort();
    if (!require(port != 0, QStringLiteral("availability error: не удалось получить свободный порт"))) return false;
    OllamaProvider unreachable(QStringLiteral("http://127.0.0.1:%1").arg(port), 1500);
    const AvailabilityResult connectionError = runCheckAvailability(unreachable);
    return require(connectionError.called && !connectionError.available && !connectionError.errorMessage.isEmpty(),
                   QStringLiteral("availability error: недоступный порт не распознан"));
}

bool testFetchAvailableModels()
{
    FakeOllamaServer server(200, R"({"models":[{"name":"qwen2.5:3b"},{"name":"llama3:8b"},{"noname":true},{"name":""}]})");
    if (!require(server.start(), QStringLiteral("models: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const ModelsResult result = runFetchModels(provider);
    return require(result.called, QStringLiteral("models: callback не вызван"))
           && require(result.errorMessage.isEmpty(), QStringLiteral("models: неожиданная ошибка: %1").arg(result.errorMessage))
           && require(result.models == QStringList({QStringLiteral("qwen2.5:3b"), QStringLiteral("llama3:8b")}),
                      QStringLiteral("models: список моделей разобран неверно"))
           && require(server.lastRequest().contains(QByteArrayLiteral("/api/tags")), QStringLiteral("models: запрошен неверный endpoint"));
}

bool testFetchAvailableModelsWithMalformedBody()
{
    FakeOllamaServer server(200, QByteArrayLiteral("not json at all"));
    if (!require(server.start(), QStringLiteral("models malformed: не удалось запустить fake server"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(server.port()));
    const ModelsResult result = runFetchModels(provider);
    return require(result.called && result.models.isEmpty(), QStringLiteral("models malformed: получен непустой список"))
           && require(!result.errorMessage.isEmpty(), QStringLiteral("models malformed: ошибка не сообщена"));
}

bool testFetchAvailableModelsWhenUnreachable()
{
    const quint16 port = closedPort();
    if (!require(port != 0, QStringLiteral("models unreachable: не удалось получить свободный порт"))) return false;
    OllamaProvider provider(QStringLiteral("http://127.0.0.1:%1").arg(port), 1500);
    const ModelsResult result = runFetchModels(provider);
    return require(result.called && result.models.isEmpty() && !result.errorMessage.isEmpty(),
                   QStringLiteral("models unreachable: недоступный порт не распознан"));
}

bool testErrorParsing()
{
    return require(OllamaProvider::parseOllamaError(R"({"error":"model not found"})") == QStringLiteral("model not found"),
                   QStringLiteral("error: JSON-поле error не извлечено"))
           && require(OllamaProvider::parseOllamaError(QByteArrayLiteral("  plain text failure  ")) == QStringLiteral("plain text failure"),
                      QStringLiteral("error: текстовое тело не использовано как сообщение"))
           && require(OllamaProvider::parseOllamaError(R"({"code":500})") == QStringLiteral("{\"code\":500}"),
                      QStringLiteral("error: JSON без поля error должен возвращаться как есть"));
}

bool testRequestBodyWithoutSystemPrompt()
{
    const QByteArray body = OllamaProvider::buildRequestBody({QString(), QStringLiteral("prompt"), QStringLiteral("model"), 0.2});
    return require(!body.contains(QByteArrayLiteral("\"system\"")), QStringLiteral("body: пустой системный промпт попал в запрос"))
           && require(body.contains(QByteArrayLiteral("\"prompt\":\"prompt\"")), QStringLiteral("body: пользовательский промпт потерян"));
}

bool testResponseTextParsing()
{
    return require(OllamaProvider::parseResponseText(R"({"response":"  {\"issues\":[]}  "})") == QStringLiteral("{\"issues\":[]}"),
                   QStringLiteral("response: текст не извлечён или не обрезан"))
           && require(OllamaProvider::parseResponseText(R"({"response":42})").isEmpty(), QStringLiteral("response: нестроковое поле принято"))
           && require(OllamaProvider::parseResponseText(QByteArrayLiteral("[]")).isEmpty(), QStringLiteral("response: не-объект принят"));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    return testProviderNameAndDefaults() && testBaseUrlNormalisation() && testTimeoutClamping()
           && testAvailabilityWhenServerAnswers() && testAvailabilityWhenServerFails()
           && testFetchAvailableModels() && testFetchAvailableModelsWithMalformedBody()
           && testFetchAvailableModelsWhenUnreachable() && testErrorParsing()
           && testRequestBodyWithoutSystemPrompt() && testResponseTextParsing() ? 0 : 1;
}

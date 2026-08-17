#include "analysis/analysis_engine.h"
#include "providers/ai_provider.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTextStream>

class MockAIProvider final : public AIProvider
{
public:
    enum class Mode { Success, ProviderError, InvalidJson };

    static AIResponse successResponse(int index)
    {
        const QString paragraphId = QStringLiteral("P00%1").arg(index + 1);
        return {true, QStringLiteral(R"({"issues":[{"id":"ISSUE_%1","paragraph_id":"%2","category":"style","severity":"medium","confidence":0.87,"original":"Текст","problem":"Проблема","recommendation":"Рекомендация"}]})").arg(index + 1).arg(paragraphId), {}};
    }

    explicit MockAIProvider(QVector<Mode> modes) : m_modes(std::move(modes)) {}

    void analyze(const AIRequest &request, std::function<void(AIResponse)> callback) override
    {
        requests.append(request);
        const int index = requests.size() - 1;
        const Mode mode = index < m_modes.size() ? m_modes.at(index) : Mode::Success;
        if (mode == Mode::ProviderError) {
            callback({false, {}, QStringLiteral("Тестовая ошибка провайдера")});
        } else if (mode == Mode::InvalidJson) {
            callback({true, QStringLiteral("not json"), {}});
        } else {
            callback(successResponse(index));
        }
    }

    QString providerName() const override { return QStringLiteral("Mock"); }

    QVector<AIRequest> requests;

private:
    QVector<Mode> m_modes;
};

namespace {
bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

Document fixtureDocument()
{
    return {QStringLiteral("fixture.docx"), QStringLiteral("Fixture"),
            {{"P001", "Первый текст.", 0, 0, {}}, {"P002", "Второй текст.", 1, 1, {}}, {"P003", "Третий текст.", 2, 2, {}}}};
}

AnalysisResult run(AnalysisEngine &engine, MockAIProvider &provider)
{
    QEventLoop loop;
    AnalysisResult result;
    QObject::connect(&engine, &AnalysisEngine::analysisFinished, &loop, [&result, &loop](const AnalysisResult &finished) {
        result = finished;
        loop.quit();
    });
    engine.start(fixtureDocument(), provider, QStringLiteral("mock-model"));
    loop.exec();
    return result;
}

bool testSuccessfulOrchestration()
{
    MockAIProvider provider({MockAIProvider::Mode::Success, MockAIProvider::Mode::Success, MockAIProvider::Mode::Success});
    AnalysisEngine engine(DocumentChunker({100}));
    const AnalysisResult result = run(engine, provider);
    if (!require(provider.requests.size() == 3, QStringLiteral("success: не каждый chunk создал AIRequest"))) return false;
    if (!require(result.issues.size() == 3 && result.processedChunks == 3 && result.processedParagraphs == 3, QStringLiteral("success: итоговый результат объединён неверно"))) return false;
    for (int index = 0; index < provider.requests.size(); ++index) {
        const AIRequest &request = provider.requests.at(index);
        if (!require(request.systemPrompt.contains(QStringLiteral("JSON")) && request.userPrompt.contains(QStringLiteral("C00%1").arg(index + 1)) && request.userPrompt.contains(QStringLiteral("P00%1").arg(index + 1)), QStringLiteral("success: PromptBuilder не передал chunk"))) return false;
    }
    return require(result.issues.at(2).paragraphId == QStringLiteral("P003"), QStringLiteral("success: paragraphId потерян"));
}

bool testPartialProviderFailure()
{
    MockAIProvider provider({MockAIProvider::Mode::Success, MockAIProvider::Mode::ProviderError, MockAIProvider::Mode::Success});
    AnalysisEngine engine;
    const AnalysisResult result = run(engine, provider);
    return require(provider.requests.size() == 3, QStringLiteral("provider failure: очередь прервана"))
           && require(result.issues.size() == 2 && result.errors.size() == 1 && result.processedChunks == 3
                      && result.processedParagraphs == 3 && result.isPartial(),
                      QStringLiteral("provider failure: частичный результат неверен"));
}

// Reports every chunk through a queued connection, so the caller can cancel the
// engine between chunks the way the UI does.
class DeferredAIProvider final : public QObject, public AIProvider
{
public:
    void analyze(const AIRequest &request, std::function<void(AIResponse)> callback) override
    {
        const int index = requestCount++;
        QMetaObject::invokeMethod(this, [callback, index]() {
            callback(MockAIProvider::successResponse(index));
        }, Qt::QueuedConnection);
    }

    QString providerName() const override { return QStringLiteral("Deferred"); }

    int requestCount = 0;
};

bool testCancelStopsQueue()
{
    DeferredAIProvider provider;
    AnalysisEngine engine(DocumentChunker({100}));
    QEventLoop loop;
    AnalysisResult result;
    QObject::connect(&engine, &AnalysisEngine::analysisFinished, &loop, [&result, &loop](const AnalysisResult &finished) {
        result = finished;
        loop.quit();
    });
    QObject::connect(&engine, &AnalysisEngine::chunkCompleted, &engine, [&engine](int) { engine.cancel(); });
    engine.start(fixtureDocument(), provider, QStringLiteral("mock-model"));
    if (!require(engine.isRunning(), QStringLiteral("cancel: движок не отмечен работающим после start"))) return false;
    loop.exec();
    return require(result.cancelled && result.isPartial(), QStringLiteral("cancel: результат не отмечен отменённым"))
           && require(provider.requestCount == 1, QStringLiteral("cancel: после отмены отправлены лишние запросы"))
           && require(result.processedChunks == 1, QStringLiteral("cancel: обработано больше chunk, чем до отмены"))
           && require(!engine.isRunning(), QStringLiteral("cancel: движок остался в работающем состоянии"));
}

bool testStartIgnoredWhileRunning()
{
    DeferredAIProvider provider;
    AnalysisEngine engine(DocumentChunker({100}));
    QEventLoop loop;
    int startedSignals = 0;
    QObject::connect(&engine, &AnalysisEngine::analysisStarted, &engine, [&startedSignals](int) { ++startedSignals; });
    QObject::connect(&engine, &AnalysisEngine::analysisFinished, &loop, &QEventLoop::quit);
    engine.start(fixtureDocument(), provider, QStringLiteral("mock-model"));
    engine.start(fixtureDocument(), provider, QStringLiteral("mock-model"));
    loop.exec();
    return require(startedSignals == 1, QStringLiteral("reentrant start: второй start не проигнорирован"))
           && require(provider.requestCount == 3, QStringLiteral("reentrant start: количество запросов изменилось"));
}

bool testEmptyDocumentFinishesImmediately()
{
    MockAIProvider provider({});
    AnalysisEngine engine;
    QEventLoop loop;
    AnalysisResult result;
    int totalChunks = -1;
    QObject::connect(&engine, &AnalysisEngine::analysisStarted, &engine, [&totalChunks](int total) { totalChunks = total; });
    QObject::connect(&engine, &AnalysisEngine::analysisFinished, &loop, [&result, &loop](const AnalysisResult &finished) {
        result = finished;
        loop.quit();
    });
    engine.start(Document{}, provider, QStringLiteral("mock-model"));
    loop.exec();
    return require(totalChunks == 0, QStringLiteral("empty: сообщено ненулевое число chunk"))
           && require(provider.requests.isEmpty(), QStringLiteral("empty: провайдер вызван для пустого документа"))
           && require(result.issues.isEmpty() && result.errors.isEmpty() && !result.isPartial(),
                      QStringLiteral("empty: результат пустого документа неверен"));
}

bool testPartialParseFailure()
{
    MockAIProvider provider({MockAIProvider::Mode::Success, MockAIProvider::Mode::InvalidJson, MockAIProvider::Mode::Success});
    AnalysisEngine engine;
    const AnalysisResult result = run(engine, provider);
    return require(provider.requests.size() == 3, QStringLiteral("parse failure: очередь прервана"))
           && require(result.issues.size() == 2 && result.errors.size() == 1 && result.errors.at(0).chunkId == QStringLiteral("C002"), QStringLiteral("parse failure: ошибка chunk не зафиксирована"));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    return testSuccessfulOrchestration() && testPartialProviderFailure() && testPartialParseFailure()
           && testCancelStopsQueue() && testStartIgnoredWhileRunning() && testEmptyDocumentFinishesImmediately() ? 0 : 1;
}

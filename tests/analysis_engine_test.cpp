#include "analysis/analysis_engine.h"
#include "providers/ai_provider.h"

#include "async_test_support.h"
#include "test_fixtures.h"
#include "test_support.h"

class MockAIProvider final : public AIProvider
{
public:
    enum class Mode { Success, ProviderError, InvalidJson };

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
            const QString paragraphId = QStringLiteral("P00%1").arg(index + 1);
            callback({true, QStringLiteral(R"({"issues":[{"id":"ISSUE_%1","paragraph_id":"%2","category":"style","severity":"medium","confidence":0.87,"original":"Текст","problem":"Проблема","recommendation":"Рекомендация"}]})").arg(index + 1).arg(paragraphId), {}});
        }
    }

    QString providerName() const override { return QStringLiteral("Mock"); }

    QVector<AIRequest> requests;

private:
    QVector<Mode> m_modes;
};

namespace {
using TestSupport::require;

constexpr int kAnalysisTimeoutMs = 15000;

Document fixtureDocument()
{
    return TestFixtures::document({TestFixtures::paragraph("P001", "Первый текст.", 0, 0),
                                   TestFixtures::paragraph("P002", "Второй текст.", 1, 1),
                                   TestFixtures::paragraph("P003", "Третий текст.", 2, 2)});
}

AnalysisResult run(AnalysisEngine &engine, MockAIProvider &provider)
{
    AnalysisResult result;
    TestSupport::awaitCallback(kAnalysisTimeoutMs, [&](const std::function<void()> &done) {
        QObject::connect(&engine, &AnalysisEngine::analysisFinished, &engine, [&result, done](const AnalysisResult &finished) {
            result = finished;
            done();
        });
        engine.start(fixtureDocument(), provider, QStringLiteral("mock-model"));
    });
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

bool testPartialParseFailure()
{
    MockAIProvider provider({MockAIProvider::Mode::Success, MockAIProvider::Mode::InvalidJson, MockAIProvider::Mode::Success});
    AnalysisEngine engine;
    const AnalysisResult result = run(engine, provider);
    return require(provider.requests.size() == 3, QStringLiteral("parse failure: очередь прервана"))
           && require(result.issues.size() == 2 && result.errors.size() == 1 && result.errors.at(0).chunkId == QStringLiteral("C002"), QStringLiteral("parse failure: ошибка chunk не зафиксирована"));
}
}

TEST_SUPPORT_MAIN(testSuccessfulOrchestration, testPartialProviderFailure, testPartialParseFailure)

#include "analysis/prompt_builder.h"
#include "analysis/response_parser.h"
#include "providers/ollama_provider.h"

#include "async_test_support.h"
#include "test_fixtures.h"
#include "test_support.h"

#include <QTextStream>

namespace {
using TestSupport::require;
using TestSupport::runAnalyze;

const char kEnableEnv[] = "OLLAMA_INTEGRATION_TESTS";
const char kModelEnv[] = "OLLAMA_MODEL";
const char kBaseUrlEnv[] = "OLLAMA_BASE_URL";
const char kDefaultModel[] = "qwen2.5:3b";
const char kDefaultBaseUrl[] = "http://127.0.0.1:11434";
constexpr int kProbeTimeoutMs = 15000;
constexpr int kAnalyzeTimeoutMs = 300000;

DocumentChunk fixtureChunk()
{
    return TestFixtures::chunk(QStringLiteral("C001"), 0, 2,
                               {{"P001", "Современная политическая система полностью зависит от цифровых технологий."},
                                {"P002", "Безусловно, сегодня абсолютно все процессы в обществе цифровизируются."}});
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTextStream out(stdout);

    const QString enable = qEnvironmentVariable(kEnableEnv);
    if (enable != QLatin1String("1") && enable != QLatin1String("true")) {
        out << "SKIPPED: установите OLLAMA_INTEGRATION_TESTS=1 для запуска." << Qt::endl;
        return 0;
    }

    const QString baseUrl = qEnvironmentVariable(kBaseUrlEnv).isEmpty() ? QString::fromLatin1(kDefaultBaseUrl) : qEnvironmentVariable(kBaseUrlEnv);
    const QString model = qEnvironmentVariable(kModelEnv).isEmpty() ? QString::fromLatin1(kDefaultModel) : qEnvironmentVariable(kModelEnv);

    OllamaProvider provider(baseUrl, 300000);

    bool available = false;
    QString availabilityError;
    TestSupport::awaitCallback(kProbeTimeoutMs, [&](const std::function<void()> &done) {
        provider.checkAvailability([&available, &availabilityError, done](bool ok, const QString &error) {
            available = ok;
            availabilityError = error;
            done();
        });
    });
    if (!available) {
        out << "SKIPPED: Ollama недоступна (" << availabilityError << ")." << Qt::endl;
        return 0;
    }

    QStringList models;
    QString modelsError;
    TestSupport::awaitCallback(kProbeTimeoutMs, [&](const std::function<void()> &done) {
        provider.fetchAvailableModels([&models, &modelsError, done](QStringList names, const QString &error) {
            models = names;
            modelsError = error;
            done();
        });
    });
    if (!models.contains(model)) {
        out << "SKIPPED: модель '" << model << "' не установлена. Доступные: " << models.join(QStringLiteral(", ")) << Qt::endl;
        return 0;
    }

    const PromptBuilder builder;
    const DocumentChunk chunk = fixtureChunk();
    const AIRequest request{builder.buildSystemPrompt(), builder.buildAnalysisPrompt(chunk), model, 0.2};

    const AIResponse aiResponse = runAnalyze(provider, request, kAnalyzeTimeoutMs);
    if (!require(aiResponse.success, QStringLiteral("integration: провайдер не вернул ответ: %1").arg(aiResponse.errorMessage))) return 1;

    const ParseResult parsed = ResponseParser().parse(aiResponse.rawText, chunk);
    if (!require(parsed.success, QStringLiteral("integration: ResponseParser не разобрал ответ модели: %1").arg(parsed.errorMessage))) return 1;

    out << "OK: обработано абзацев: " << parsed.result.processedParagraphs
        << ", найдено issues: " << parsed.result.issues.size() << Qt::endl;
    for (const Issue &issue : parsed.result.issues) {
        out << "  " << issue.id << " [" << issue.paragraphId << "] " << issue.problem << Qt::endl;
    }
    return 0;
}

#include "analysis/prompt_builder.h"
#include "analysis/response_parser.h"
#include "providers/ollama_provider.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QTextStream>
#include <QTimer>

#include <utility>

namespace {
const char kEnableEnv[] = "OLLAMA_INTEGRATION_TESTS";
const char kModelEnv[] = "OLLAMA_MODEL";
const char kBaseUrlEnv[] = "OLLAMA_BASE_URL";
const char kDefaultModel[] = "qwen2.5:3b";
const char kDefaultBaseUrl[] = "http://127.0.0.1:11434";

bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

DocumentChunk fixtureChunk()
{
    return {"C001", 0, 2,
            {"P001", "P002"},
            {{"P001", "Современная политическая система полностью зависит от цифровых технологий."},
             {"P002", "Безусловно, сегодня абсолютно все процессы в обществе цифровизируются."}},
            QString()};
}

AIResponse runAnalyze(OllamaProvider &provider, const AIRequest &request, int timeoutMs = 300000)
{
    QEventLoop loop;
    AIResponse response;
    provider.analyze(request, [&](AIResponse result) {
        response = result;
        loop.quit();
    });
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
    return response;
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

    QEventLoop loop;
    bool available = false;
    QString availabilityError;
    provider.checkAvailability([&](bool ok, const QString &error) {
        available = ok;
        availabilityError = error;
        loop.quit();
    });
    QTimer::singleShot(15000, &loop, &QEventLoop::quit);
    loop.exec();
    if (!available) {
        out << "SKIPPED: Ollama недоступна (" << availabilityError << ")." << Qt::endl;
        return 0;
    }

    QStringList models;
    QString modelsError;
    QEventLoop modelsLoop;
    provider.fetchAvailableModels([&](QStringList names, const QString &error) {
        models = names;
        modelsError = error;
        modelsLoop.quit();
    });
    QTimer::singleShot(15000, &modelsLoop, &QEventLoop::quit);
    modelsLoop.exec();
    if (!models.contains(model)) {
        out << "SKIPPED: модель '" << model << "' не установлена. Доступные: " << models.join(QStringLiteral(", ")) << Qt::endl;
        return 0;
    }

    const PromptBuilder builder;
    const DocumentChunk chunk = fixtureChunk();
    const AIRequest request{builder.buildSystemPrompt(), builder.buildAnalysisPrompt(chunk), model, 0.2};

    const AIResponse aiResponse = runAnalyze(provider, request);
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

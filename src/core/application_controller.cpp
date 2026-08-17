#include "core/application_controller.h"

#include "analysis/analysis_engine.h"
#include "document/docx_reader.h"
#include "providers/ollama_provider.h"

#include <QFileInfo>

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
{
}

ApplicationController::~ApplicationController() = default;

bool ApplicationController::startAnalysis(const QString &docxPath, const QString &model, const QString &baseUrl)
{
    if (m_running) {
        emit analysisFailed(QStringLiteral("Анализ уже запущен."));
        return false;
    }

    const QFileInfo fileInfo(docxPath);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        emit analysisFailed(QStringLiteral("Файл не найден: %1").arg(docxPath));
        return false;
    }
    if (fileInfo.suffix().compare(QLatin1String("docx"), Qt::CaseInsensitive) != 0) {
        emit analysisFailed(QStringLiteral("Поддерживаются только файлы .docx."));
        return false;
    }

    DocxReader reader;
    const Document document = reader.read(docxPath);
    if (document.isEmpty()) {
        const QString error = reader.errorString().isEmpty()
                                  ? QStringLiteral("Документ не содержит абзацев.")
                                  : reader.errorString();
        emit analysisFailed(error);
        return false;
    }

    // Создаём OllamaProvider (он будет жить пока жив контроллер)
    m_provider = std::make_unique<OllamaProvider>(baseUrl);

    // Создаём Engine, если ещё не создан, или пересоздаём на всякий случай
    m_engine = std::make_unique<AnalysisEngine>();

    // Подключаем сигналы Engine к сигналам контроллера
    connect(m_engine.get(), &AnalysisEngine::analysisStarted, this, &ApplicationController::analysisStarted);
    connect(m_engine.get(), &AnalysisEngine::progressChanged, this, &ApplicationController::progressChanged);
    connect(m_engine.get(), &AnalysisEngine::chunkCompleted, this, &ApplicationController::chunkCompleted);
    connect(m_engine.get(), &AnalysisEngine::issueFound, this, &ApplicationController::issueFound);
    connect(m_engine.get(), &AnalysisEngine::chunkFailed, this, &ApplicationController::chunkFailed);
    connect(m_engine.get(), &AnalysisEngine::analysisFinished, this, [this](const AnalysisResult &result) {
        m_running = false;
        emit analysisFinished(result);
    });

    m_running = true;
    // model может быть пустой — OllamaProvider сам отклонит; температура по умолчанию 0.2
    m_engine->start(document, *m_provider, model.isEmpty() ? QStringLiteral("qwen2.5:3b") : model, 0.2);
    return true;
}

void ApplicationController::cancel()
{
    if (m_engine) {
        m_engine->cancel();
    }
    if (m_provider) {
        m_provider->cancelAll();
    }
    // m_running сбрасывается только по сигналу analysisFinished
}

bool ApplicationController::isRunning() const
{
    return m_running;
}

#include "analysis/analysis_engine.h"
#include "providers/ai_provider.h"
#include <QDebug>
#include <QMetaObject>
#include <utility>

AnalysisEngine::AnalysisEngine(DocumentChunker chunker, PromptBuilder promptBuilder,
                               ResponseParser responseParser, QObject *parent)
    : QObject(parent),
      m_chunker(std::move(chunker)),
      m_promptBuilder(std::move(promptBuilder)),
      m_responseParser(std::move(responseParser))
{
}

bool AnalysisEngine::start(const Document &document, AIProvider &provider, const QString &model, double temperature)
{
    if (m_running) {
        emit startRejected(QStringLiteral("Анализ уже выполняется."));
        return false;
    }
    m_chunks = m_chunker.split(document);
    if (m_chunks.isEmpty()) {
        emit startRejected(QStringLiteral("Документ не содержит текста для анализа."));
        return false;
    }
    m_result = {};
    m_provider = &provider;
    m_model = model;
    m_temperature = temperature;
    m_nextChunkIndex = 0;
    m_cancelRequested = false;
    m_running = true;
    emit analysisStarted(m_chunks.size());
    QMetaObject::invokeMethod(this, &AnalysisEngine::processNextChunk, Qt::QueuedConnection);
    return true;
}

void AnalysisEngine::cancel()
{
    if (m_running) m_cancelRequested = true;
}

bool AnalysisEngine::isRunning() const { return m_running; }

void AnalysisEngine::processNextChunk()
{
    if (!m_running) return;
    if (m_cancelRequested || m_nextChunkIndex >= m_chunks.size()) {
        m_result.cancelled = m_cancelRequested;
        finish();
        return;
    }

    const int chunkIndex = m_nextChunkIndex++;
    const DocumentChunk chunk = m_chunks.at(chunkIndex);
    AIRequest request{m_promptBuilder.buildSystemPrompt(), m_promptBuilder.buildAnalysisPrompt(chunk), m_model, m_temperature};
    m_provider->analyze(request, [this, chunkIndex](AIResponse response) {
        QMetaObject::invokeMethod(this, [this, chunkIndex, response = std::move(response)]() mutable {
            handleResponse(chunkIndex, std::move(response));
        }, Qt::QueuedConnection);
    });
}

void AnalysisEngine::handleResponse(int chunkIndex, AIResponse response)
{
    if (!m_running) return;
    if (chunkIndex < 0 || chunkIndex >= m_chunks.size()) {
        qWarning("AnalysisEngine: получен ответ для неизвестного chunk %d, ответ отброшен", chunkIndex);
        return;
    }
    const DocumentChunk &chunk = m_chunks.at(chunkIndex);
    ++m_result.processedChunks;
    m_result.processedParagraphs += chunk.paragraphIds.size();
    if (!response.success) {
        const QString message = response.errorMessage.isEmpty() ? QStringLiteral("Провайдер вернул неизвестную ошибку.") : response.errorMessage;
        m_result.errors.append({chunk.id, message});
        emit chunkFailed(chunkIndex, message);
    } else {
        const ParseResult parsed = m_responseParser.parse(response.rawText, chunk);
        if (!parsed.success) {
            m_result.errors.append({chunk.id, parsed.errorMessage});
            emit chunkFailed(chunkIndex, parsed.errorMessage);
        } else {
            m_result.issues += parsed.result.issues;
            for (const Issue &issue : parsed.result.issues) emit issueFound(issue);
            emit chunkCompleted(chunkIndex);
        }
    }
    emit progressChanged(chunkIndex + 1, m_chunks.size());
    QMetaObject::invokeMethod(this, &AnalysisEngine::processNextChunk, Qt::QueuedConnection);
}

void AnalysisEngine::finish()
{
    if (!m_running) return;
    m_running = false;
    emit analysisFinished(m_result);
}

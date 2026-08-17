#ifndef ANALYSIS_ENGINE_H
#define ANALYSIS_ENGINE_H

#include "analysis/prompt_builder.h"
#include "analysis/response_parser.h"
#include "core/ai_types.h"
#include "document/document_chunker.h"

#include <QObject>

class AIProvider;

class AnalysisEngine : public QObject
{
    Q_OBJECT

public:
    explicit AnalysisEngine(DocumentChunker chunker = DocumentChunker{}, PromptBuilder promptBuilder = {},
                            ResponseParser responseParser = {}, QObject *parent = nullptr);

    bool start(const Document &document, AIProvider &provider, const QString &model = {}, double temperature = 0.2);
    void cancel();
    bool isRunning() const;

signals:
    void analysisStarted(int totalChunks);
    void startRejected(const QString &message);
    void progressChanged(int current, int total);
    void chunkCompleted(int chunkIndex);
    void issueFound(const Issue &issue);
    void chunkFailed(int chunkIndex, const QString &message);
    void analysisFinished(const AnalysisResult &result);

private:
    void processNextChunk();
    void handleResponse(int chunkIndex, AIResponse response);
    void finish();

    DocumentChunker m_chunker;
    PromptBuilder m_promptBuilder;
    ResponseParser m_responseParser;
    QVector<DocumentChunk> m_chunks;
    AnalysisResult m_result;
    AIProvider *m_provider = nullptr;
    QString m_model;
    double m_temperature = 0.2;
    int m_nextChunkIndex = 0;
    bool m_running = false;
    bool m_cancelRequested = false;
};

#endif // ANALYSIS_ENGINE_H

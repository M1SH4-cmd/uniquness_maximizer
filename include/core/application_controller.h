#ifndef APPLICATION_CONTROLLER_H
#define APPLICATION_CONTROLLER_H

#include "core/analysis_types.h"

#include <QObject>
#include <QString>
#include <memory>

class AnalysisEngine;
class OllamaProvider;

class ApplicationController : public QObject
{
    Q_OBJECT

public:
    explicit ApplicationController(QObject *parent = nullptr);
    ~ApplicationController() override;

    bool startAnalysis(const QString &docxPath, const QString &model, const QString &baseUrl);
    void cancel();
    bool isRunning() const;

signals:
    void analysisStarted(int totalChunks);
    void progressChanged(int current, int total);
    void chunkCompleted(int chunkIndex);
    void issueFound(const Issue &issue);
    void chunkFailed(int chunkIndex, const QString &message);
    void analysisFinished(const AnalysisResult &result);
    void analysisFailed(const QString &message);

private:
    std::unique_ptr<OllamaProvider> m_provider;
    std::unique_ptr<AnalysisEngine> m_engine;
    bool m_running = false;
};

#endif // APPLICATION_CONTROLLER_H

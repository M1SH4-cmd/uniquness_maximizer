#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QList>
#include <string>

class QLabel;
class QPushButton;
class QProgressBar;
class QTextEdit;
class QStackedWidget;
class QFrame;
class QComboBox;

struct ApiKeyEntry {
    QString id;
    QString provider;
    QString key;
    bool selected = false;
};

class ApplicationController;
struct AnalysisResult;
struct Issue;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr, const std::string &path = "");
    ~MainWindow() override;

private slots:
    void chooseDocument();
    void startReview();
    void configureApiKey();

    void onAnalysisStarted(int totalChunks);
    void onProgressChanged(int current, int total);
    void onChunkCompleted(int chunkIndex);
    void onIssueFound(const Issue &issue);
    void onChunkFailed(int chunkIndex, const QString &message);
    void onAnalysisFinished(const AnalysisResult &result);
    void onAnalysisFailed(const QString &message);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void setDocument(const QString &filePath);
    void updateRequirements();
    QString keysDirectoryPath() const;
    QString keyFilePath(const QString &id) const;
    QString keySummary(const QString &provider, const QString &key) const;
    QList<ApiKeyEntry> loadKeys() const;
    bool saveKey(const ApiKeyEntry &entry) const;
    bool removeKey(const QString &id) const;
    void selectKey(const QString &id);
    void updateActiveKey(const QList<ApiKeyEntry> &entries);
    void setControlsRunning(bool running);

    ApplicationController *controller = nullptr;

    QString documentPath;
    QString apiKey;
    QString activeKeyId;
    QLabel *documentNameLabel = nullptr;
    QLabel *documentMetaLabel = nullptr;
    QLabel *statusLabel = nullptr;
    QLabel *apiStatusLabel = nullptr;
    QComboBox *modelComboBox = nullptr;
    QLabel *reviewSummaryLabel = nullptr;
    QLabel *requirementsTitleLabel = nullptr;
    QFrame *requirementsPanel = nullptr;
    QPushButton *fileRequirementButton = nullptr;
    QPushButton *keyRequirementButton = nullptr;
    QTextEdit *previewText = nullptr;
    QPushButton *selectButton = nullptr;
    QPushButton *reviewButton = nullptr;
    QProgressBar *progressBar = nullptr;
    QStackedWidget *contentStack = nullptr;
};

#endif // MAINWINDOW_H

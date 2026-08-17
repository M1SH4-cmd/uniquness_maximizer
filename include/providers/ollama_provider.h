#ifndef OLLAMA_PROVIDER_H
#define OLLAMA_PROVIDER_H

#include "providers/ai_provider.h"

#include <QHash>
#include <QNetworkAccessManager>
#include <QObject>
#include <QString>
#include <QStringList>

#include <functional>

class QNetworkReply;

class OllamaProvider final : public QObject, public AIProvider
{
    Q_OBJECT

public:
    explicit OllamaProvider(QString baseUrl = QStringLiteral("http://127.0.0.1:11434"),
                            int timeoutMs = 120000,
                            QObject *parent = nullptr);

    void analyze(const AIRequest &request, std::function<void(AIResponse)> callback) override;
    QString providerName() const override;

    void checkAvailability(std::function<void(bool available, const QString &errorMessage)> callback);
    void fetchAvailableModels(std::function<void(QStringList models, const QString &errorMessage)> callback);
    void cancelAll();

    QString baseUrl() const;
    void setBaseUrl(const QString &url);
    int timeoutMs() const;
    void setTimeoutMs(int ms);

    static QByteArray buildRequestBody(const AIRequest &request);
    static QString parseResponseText(const QByteArray &body);
    static QString parseOllamaError(const QByteArray &body);

private:
    void handleReplyFinished(QNetworkReply *reply);

    QNetworkAccessManager m_network;
    QString m_baseUrl;
    int m_timeoutMs;
    QHash<QNetworkReply *, std::function<void(QNetworkReply *)>> m_pending;
};

#endif // OLLAMA_PROVIDER_H

// ##Script function and purpose: Defines the network client for interacting with a Llama.cpp REST API using SSE streaming.
#pragma once
#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// ##Class purpose: Wrapper for asynchronous streaming network requests to the local LLM server.
class LlamaClient : public QObject {
    Q_OBJECT
public:
    // ##Method purpose: Constructor.
    explicit LlamaClient(QObject *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~LlamaClient() override = default;

    // ##Method purpose: Sets the base URL of the Llama.cpp server.
    void setEndpointUrl(const QString &url);
    
    // ##Method purpose: Returns the current base URL.
    QString endpointUrl() const;

    // ##Method purpose: Trigger an inline completion request (infill).
    virtual void requestCompletion(const QString &prefix, const QString &suffix);
    
    // ##Method purpose: Trigger a chat completion request.
    virtual void requestChat(const QJsonArray &messages);

    // ##Method purpose: Prepares and sends an HTTP POST request for refactoring.
    virtual void requestRefactor(const QString &promptText);

Q_SIGNALS:
    // ##Method purpose: Emitted when an inline completion response stream completes.
    void completionReceived(const QString &text);
    
    // ##Method purpose: Emitted when a new chat token is received via SSE.
    void chatTokenReceived(const QString &token);

    // ##Method purpose: Emitted when the chat response stream completes.
    void chatResponseFinished();
    
    // ##Method purpose: Emitted when a refactor response completes.
    void refactorReceived(const QString &text);
    
    // ##Method purpose: Emitted when a network or parsing error occurs.
    void errorOccurred(const QString &errorString);

    // ##Method purpose: Emitted when a security warning occurs.
    void warningOccurred(const QString &warningString);

private Q_SLOTS:
    // ##Method purpose: Callback for when the completion HTTP request receives data chunks.
    void onCompletionReadyRead();
    
    // ##Method purpose: Callback for when the completion HTTP request finishes.
    void onCompletionFinished();

    // ##Method purpose: Callback for when the chat HTTP request receives SSE chunks.
    void onChatReadyRead();
    
    // ##Method purpose: Callback for when the chat HTTP request finishes.
    void onChatFinished();

    // ##Method purpose: Callback for when the refactor HTTP request receives chunks.
    void onRefactorReadyRead();
    
    // ##Method purpose: Callback for when the refactor HTTP request finishes.
    void onRefactorFinished();

private:
    // ##Method purpose: Emits a warning if the endpoint scheme is insecure and non-loopback.
    void checkInsecureEndpoint(const QString &scheme, const QString &host);

    QNetworkAccessManager *m_nam;
    QString m_endpointUrl = QStringLiteral("http://localhost:8080");
    QString m_completionBuffer;
    QString m_refactorBuffer;
    QNetworkReply *m_chatReply = nullptr;
    QNetworkReply *m_completionReply = nullptr;
    QNetworkReply *m_refactorReply = nullptr;
    bool m_insecureWarningEmitted = false;
};

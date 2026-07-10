// ##Script function and purpose: Implements the asynchronous streaming network client to interface with Llama.cpp endpoints.
#include "LlamaClient.h"
#include <QNetworkRequest>
#include <QJsonArray>
#include <QJsonObject>
#include <QUrl>
#include <QDebug>

#include <KSharedConfig>
#include <KConfigGroup>

// ##Method purpose: Constructor.
LlamaClient::LlamaClient(QObject *parent) 
    : QObject(parent), m_nam(new QNetworkAccessManager(this)) 
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group = config->group(QStringLiteral("KDevLLM"));
    m_endpointUrl = group.readEntry("EndpointUrl", QStringLiteral("http://127.0.0.1:8080"));
}

// ##Method purpose: Update the server endpoint URL.
void LlamaClient::setEndpointUrl(const QString &url)
{
    m_endpointUrl = url;
    m_insecureWarningEmitted = false;
}

// ##Method purpose: Retrieve the server endpoint URL.
QString LlamaClient::endpointUrl() const
{
    return m_endpointUrl;
}

// ##Method purpose: Emits a warning if the endpoint scheme is insecure and non-loopback.
void LlamaClient::checkInsecureEndpoint(const QString &scheme, const QString &host)
{
    // ##Condition purpose: Warn only once per endpoint change if HTTP is used on a non-loopback address.
    if (scheme == QStringLiteral("http") && host != QStringLiteral("127.0.0.1") && host != QStringLiteral("localhost") && host != QStringLiteral("::1")) {
        // ##Condition purpose: Prevent duplicate warnings from being emitted for the same endpoint configuration.
        if (!m_insecureWarningEmitted) {
            Q_EMIT warningOccurred(QStringLiteral("Using HTTP with a non-loopback endpoint exposes your code to network interception. Consider using HTTPS."));
            m_insecureWarningEmitted = true;
        }
    }
}

// ##Method purpose: Prepares and sends an HTTP POST request to the /completion endpoint.
void LlamaClient::requestCompletion(const QString &prefix, const QString &suffix)
{
    QUrl url(m_endpointUrl + QStringLiteral("/completion"));
    checkInsecureEndpoint(url.scheme(), url.host());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));

    QJsonObject json;
    json[QStringLiteral("prompt")] = prefix;
    json[QStringLiteral("suffix")] = suffix;
    json[QStringLiteral("n_predict")] = 128;
    json[QStringLiteral("temperature")] = 0.2;
    json[QStringLiteral("stream")] = true;

    if (m_completionReply) {
        m_completionReply->abort();
    }
    
    m_completionBuffer.clear();
    m_completionReply = m_nam->post(request, QJsonDocument(json).toJson());
    connect(m_completionReply, &QNetworkReply::readyRead, this, &LlamaClient::onCompletionReadyRead);
    connect(m_completionReply, &QNetworkReply::finished, this, &LlamaClient::onCompletionFinished);
}

// ##Method purpose: Prepares and sends an HTTP POST request to the /chat/completions endpoint.
void LlamaClient::requestChat(const QJsonArray &messages)
{
    QUrl url(m_endpointUrl + QStringLiteral("/chat/completions"));
    checkInsecureEndpoint(url.scheme(), url.host());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));

    QJsonObject json;
    json[QStringLiteral("messages")] = messages;
    json[QStringLiteral("temperature")] = 0.7;
    json[QStringLiteral("stream")] = true;

    if (m_chatReply) {
        m_chatReply->abort();
    }
    
    m_chatReply = m_nam->post(request, QJsonDocument(json).toJson());
    connect(m_chatReply, &QNetworkReply::readyRead, this, &LlamaClient::onChatReadyRead);
    connect(m_chatReply, &QNetworkReply::finished, this, &LlamaClient::onChatFinished);
}

// ##Method purpose: Prepares and sends an HTTP POST request for refactoring.
void LlamaClient::requestRefactor(const QString &promptText)
{
    QUrl url(m_endpointUrl + QStringLiteral("/completion"));
    checkInsecureEndpoint(url.scheme(), url.host());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QByteArray("application/json"));

    QJsonObject json;
    json[QStringLiteral("prompt")] = promptText;
    json[QStringLiteral("n_predict")] = 512;
    json[QStringLiteral("temperature")] = 0.2;
    json[QStringLiteral("stream")] = true;

    if (m_refactorReply) {
        m_refactorReply->abort();
    }
    
    m_refactorBuffer.clear();
    m_refactorReply = m_nam->post(request, QJsonDocument(json).toJson());
    connect(m_refactorReply, &QNetworkReply::readyRead, this, &LlamaClient::onRefactorReadyRead);
    connect(m_refactorReply, &QNetworkReply::finished, this, &LlamaClient::onRefactorFinished);
}

// ##Method purpose: Parses the SSE chunks from the /completion endpoint.
void LlamaClient::onCompletionReadyRead()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    // ##Condition purpose: Ensure reply is valid.
    if (!reply) return;

    // ##Loop purpose: Read all available SSE lines.
    while (reply->canReadLine()) {
        QByteArray line = reply->readLine().trimmed();
        
        // ##Condition purpose: Parse only data lines.
        if (line.startsWith("data: ")) {
            QByteArray jsonData = line.mid(6);
            if (jsonData == "[DONE]") continue;

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (doc.isObject()) {
                m_completionBuffer += doc.object()[QStringLiteral("content")].toString();
            }
        }
    }
}

// ##Method purpose: Handles completion of the /completion request.
void LlamaClient::onCompletionFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    
    // ##Condition purpose: Handle network errors.
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            Q_EMIT errorOccurred(reply->errorString());
        }
    } else {
        Q_EMIT completionReceived(m_completionBuffer);
    }
    
    if (reply == m_completionReply) {
        m_completionReply = nullptr;
    }
    reply->deleteLater();
}

// ##Method purpose: Parses the SSE chunks from the /chat/completions endpoint.
void LlamaClient::onChatReadyRead()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    // ##Condition purpose: Ensure reply is valid.
    if (!reply) return;

    // ##Loop purpose: Read all available SSE lines.
    while (reply->canReadLine()) {
        QByteArray line = reply->readLine().trimmed();
        
        // ##Condition purpose: Parse only data lines.
        if (line.startsWith("data: ")) {
            QByteArray jsonData = line.mid(6);
            if (jsonData == "[DONE]") continue;

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (doc.isObject()) {
                QJsonArray choices = doc.object()[QStringLiteral("choices")].toArray();
                if (!choices.isEmpty()) {
                    QJsonObject delta = choices.first().toObject()[QStringLiteral("delta")].toObject();
                    QString content = delta[QStringLiteral("content")].toString();
                    if (!content.isEmpty()) {
                        Q_EMIT chatTokenReceived(content);
                    }
                }
            }
        }
    }
}

// ##Method purpose: Handles completion of the /chat/completions request.
void LlamaClient::onChatFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    
    // ##Condition purpose: Handle network errors.
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            Q_EMIT errorOccurred(reply->errorString());
        }
    } else {
        Q_EMIT chatResponseFinished();
    }
    
    if (reply == m_chatReply) {
        m_chatReply = nullptr;
    }
    reply->deleteLater();
}

// ##Method purpose: Parses the SSE chunks from the refactor endpoint.
void LlamaClient::onRefactorReadyRead()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;

    while (reply->canReadLine()) {
        QByteArray line = reply->readLine().trimmed();
        
        if (line.startsWith("data: ")) {
            QByteArray jsonData = line.mid(6);
            if (jsonData == "[DONE]") continue;

            QJsonDocument doc = QJsonDocument::fromJson(jsonData);
            if (doc.isObject()) {
                m_refactorBuffer += doc.object()[QStringLiteral("content")].toString();
            }
        }
    }
}

// ##Method purpose: Handles completion of the refactor request.
void LlamaClient::onRefactorFinished()
{
    auto *reply = qobject_cast<QNetworkReply *>(sender());
    if (!reply) return;
    
    if (reply->error() != QNetworkReply::NoError) {
        if (reply->error() != QNetworkReply::OperationCanceledError) {
            Q_EMIT errorOccurred(reply->errorString());
        }
    } else {
        // Strip out the wrapping markdown syntax from the LLM's response
        QString clean = m_refactorBuffer.trimmed();
        if (clean.startsWith(QStringLiteral("```"))) {
            int newlineIdx = clean.indexOf(QLatin1Char('\n'));
            if (newlineIdx != -1) {
                clean = clean.mid(newlineIdx + 1);
            } else {
                clean.remove(0, 3);
            }
        }
        if (clean.endsWith(QStringLiteral("```"))) {
            clean.chop(3);
        }
        clean = clean.trimmed();
        Q_EMIT refactorReceived(clean);
    }
    
    if (reply == m_refactorReply) {
        m_refactorReply = nullptr;
    }
    reply->deleteLater();
}

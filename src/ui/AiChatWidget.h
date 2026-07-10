// ##Script function and purpose: Defines the UI widget for the AI chat panel, with Markdown rendering and streaming text support.
#pragma once

#include <QWidget>
#include <QJsonArray>
#include <QUrl>

class QTextBrowser;
class AiChatInputWidget;
class QPushButton;
class QVBoxLayout;
class LlamaClient;
class ContextManager;

// ##Class purpose: Manages the chat history display and input box for interacting with the AI.
class AiChatWidget : public QWidget {
    Q_OBJECT
    friend class TestAiChatWidget;
public:
    // ##Method purpose: Constructor for the chat widget.
    explicit AiChatWidget(QWidget *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~AiChatWidget() override = default;

protected:
    // ##Method purpose: Intercepts events, specifically the Enter key for sending messages.
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    // ##Method purpose: Triggered when the user clicks Send; packages the message.
    void sendMessage(const QString &text);
    
    // ##Method purpose: Triggered when a new token arrives from the SSE stream.
    void onChatTokenReceived(const QString &token);
    
    // ##Method purpose: Triggered when the SSE stream completes.
    void onChatFinished();
    
    // ##Method purpose: Triggered when the LlamaClient encounters an error.
    void onError(const QString &error);

    // ##Method purpose: Clears the message history and resets the chat UI.
    void clearChat();

    // ##Method purpose: Securely handles clicked links to prevent arbitrary scheme execution.
    void onAnchorClicked(const QUrl &url);

private:
    // ##Method purpose: Refreshes the QTextBrowser with the accumulated markdown string.
    void renderMarkdown();
    
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    QTextBrowser *m_chatHistory;
    AiChatInputWidget *m_inputWidget;
    
    LlamaClient *m_client;
    ContextManager *m_context;
    QJsonArray m_messageHistory;
    QString m_rawMarkdown;
    QString m_currentAssistantResponse;
};

// ##Script function and purpose: Defines the UI widget for the AI chat panel, with Markdown rendering and streaming text support.
#pragma once

#include <QWidget>
#include <QJsonArray>

class QTextBrowser;
class QTextEdit;
class QPushButton;
class QVBoxLayout;
class LlamaClient;
class ContextManager;
namespace KTextEditor { class MainWindow; }

// ##Class purpose: Manages the chat history display and input box for interacting with the AI.
class AiChatWidget : public QWidget {
    Q_OBJECT
public:
    // ##Method purpose: Constructor for the chat widget.
    explicit AiChatWidget(KTextEditor::MainWindow *mainWindow, QWidget *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~AiChatWidget() override = default;

protected:
    // ##Method purpose: Intercepts events, specifically the Enter key for sending messages.
    bool eventFilter(QObject *watched, QEvent *event) override;

private Q_SLOTS:
    // ##Method purpose: Triggered when the user clicks Send; packages the message.
    void sendMessage();
    
    // ##Method purpose: Triggered when a new token arrives from the SSE stream.
    void onChatTokenReceived(const QString &token);
    
    // ##Method purpose: Triggered when the SSE stream completes.
    void onChatFinished();
    
    // ##Method purpose: Triggered when the LlamaClient encounters an error.
    void onError(const QString &error);

    // ##Method purpose: Clears the message history and resets the chat UI.
    void clearChat();

private:
    // ##Method purpose: Refreshes the QTextBrowser with the accumulated markdown string.
    void renderMarkdown();
    
    KTextEditor::MainWindow *m_mainWindow;
    QTextBrowser *m_chatHistory;
    QTextEdit *m_inputBox;
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    
    LlamaClient *m_client;
    ContextManager *m_context;
    QJsonArray m_messageHistory;
    QString m_rawMarkdown;
    QString m_currentAssistantResponse;
};

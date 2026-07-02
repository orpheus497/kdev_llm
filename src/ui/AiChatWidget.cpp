// ##Script function and purpose: Implements the UI layout and event handling for the chat widget, including SSE chunking.
#include "AiChatWidget.h"
#include "../network/LlamaClient.h"
#include "../context/ContextManager.h"

#include <KTextEditor/MainWindow>
#include <KTextEditor/View>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QJsonObject>
#include <QEvent>
#include <QKeyEvent>

// ##Method purpose: Sets up the layout, initializes UI components, and connects signals.
AiChatWidget::AiChatWidget(KTextEditor::MainWindow *mainWindow, QWidget *parent)
    : QWidget(parent)
    , m_mainWindow(mainWindow)
    , m_client(new LlamaClient(this))
    , m_context(new ContextManager(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_chatHistory = new QTextBrowser(this);
    m_chatHistory->setOpenExternalLinks(true);
    m_chatHistory->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(m_chatHistory);
    
    auto *inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(4, 4, 4, 4);
    
    m_inputBox = new QTextEdit(this);
    m_inputBox->installEventFilter(this);
    m_inputBox->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_inputBox->setMaximumHeight(80);
    m_inputBox->setMinimumHeight(40);
    inputLayout->addWidget(m_inputBox);
    
    m_sendButton = new QPushButton(QStringLiteral("Send"), this);
    inputLayout->addWidget(m_sendButton);
    
    m_newChatButton = new QPushButton(QStringLiteral("New Chat"), this);
    inputLayout->addWidget(m_newChatButton);
    
    layout->addLayout(inputLayout);
    
    connect(m_sendButton, &QPushButton::clicked, this, &AiChatWidget::sendMessage);
    connect(m_newChatButton, &QPushButton::clicked, this, &AiChatWidget::clearChat);
    connect(m_client, &LlamaClient::chatTokenReceived, this, &AiChatWidget::onChatTokenReceived);
    connect(m_client, &LlamaClient::chatResponseFinished, this, &AiChatWidget::onChatFinished);
    connect(m_client, &LlamaClient::errorOccurred, this, &AiChatWidget::onError);
    
    // Display welcome guide
    clearChat();
}

// ##Method purpose: Intercepts Enter key events on the input box to send the message.
bool AiChatWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == m_inputBox && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (!(keyEvent->modifiers() & Qt::ShiftModifier)) {
                sendMessage();
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}

// ##Method purpose: Reads user input, builds the JSON message history, and starts the stream.
void AiChatWidget::sendMessage()
{
    QString text = m_inputBox->toPlainText().trimmed();
    
    // ##Condition purpose: Prevent sending empty messages.
    if (text.isEmpty()) return;
    
    m_inputBox->clear();
    m_rawMarkdown += QStringLiteral("**You:**\n\n") + text + QStringLiteral("\n\n---\n\n**AI:**\n\n");
    renderMarkdown();
    
    // ##Condition purpose: Inject the system prompt only on the very first message.
    if (m_messageHistory.isEmpty()) {
        QString sysPrompt = m_context->buildSystemPrompt(m_mainWindow->activeView());
        QJsonObject sysMsg;
        sysMsg[QStringLiteral("role")] = QStringLiteral("system");
        sysMsg[QStringLiteral("content")] = sysPrompt;
        m_messageHistory.append(sysMsg);
    }
    
    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = text;
    m_messageHistory.append(userMsg);
    
    m_currentAssistantResponse.clear();
    m_sendButton->setEnabled(false);
    m_client->requestChat(m_messageHistory);
}

// ##Method purpose: Receives incoming streamed tokens and renders the evolving markdown.
void AiChatWidget::onChatTokenReceived(const QString &token)
{
    m_currentAssistantResponse += token;
    m_rawMarkdown += token;
    renderMarkdown();
}

// ##Method purpose: Triggered when the AI finishes responding, finalizing the message block.
void AiChatWidget::onChatFinished()
{
    m_sendButton->setEnabled(true);
    
    QJsonObject assistantMsg;
    assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
    assistantMsg[QStringLiteral("content")] = m_currentAssistantResponse;
    m_messageHistory.append(assistantMsg);
    
    m_rawMarkdown += QStringLiteral("\n\n---\n\n");
    renderMarkdown();
}

// ##Method purpose: Displays a network or parsing error in the chat log.
void AiChatWidget::onError(const QString &error)
{
    m_sendButton->setEnabled(true);
    m_rawMarkdown += QStringLiteral("\n\n**Error:** `") + error + QStringLiteral("`\n\n---\n\n");
    renderMarkdown();
}

// ##Method purpose: Refreshes the display with Qt6's native Markdown parser.
void AiChatWidget::renderMarkdown()
{
    m_chatHistory->setMarkdown(m_rawMarkdown);
    m_chatHistory->moveCursor(QTextCursor::End);
}

// ##Method purpose: Clears the message history and resets the chat UI.
void AiChatWidget::clearChat()
{
    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_rawMarkdown = QStringLiteral("# Jenova K Text\n\nWelcome to Jenova K Text, your AI Assistant for Kate and KDevelop!\n\n## Features:\n- **Chat**: Type below and press `Enter` to ask questions about your code.\n- **Refactor**: Select code, right-click (or Tools menu) and choose **AI: Refactor Selection...**\n- **Autocomplete**: Press `Ctrl+Space` while typing to get AI code suggestions.\n\n*(Note: Ensure your local Llama.cpp server is running at the configured endpoint in Settings)*\n\n---\n\n");
    renderMarkdown();
}

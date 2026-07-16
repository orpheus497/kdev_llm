// ##Script function and purpose: Implements the UI layout and event handling for the chat widget, including SSE chunking.
#include "AiChatWidget.h"
#include "AiChatInputWidget.h"
#include "../network/LlamaClient.h"
#include "../context/ContextManager.h"

#include <KTextEditor/View>
#include <QVBoxLayout>
#include <QTextBrowser>
#include <QJsonObject>
#include <QEvent>
#include <QKeyEvent>
#include <KTextEditor/Document>
#include <QDesktopServices>
#include <QUrl>
#include <QTextDocument>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>
#include <QDateTime>
#include <QTimer>

#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>

// ##Method purpose: Sets up the layout, initializes UI components, and connects signals.
AiChatWidget::AiChatWidget(QWidget *parent)
    : QWidget(parent)
    , m_client(new LlamaClient(this))
    , m_context(new ContextManager(this))
    , m_renderTimer(new QTimer(this))
{
    m_renderTimer->setSingleShot(true);
    m_renderTimer->setInterval(50);
    connect(m_renderTimer, &QTimer::timeout, this, &AiChatWidget::renderMarkdown);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    
    m_chatHistory = new QTextBrowser(this);
    m_chatHistory->setOpenExternalLinks(false);
    m_chatHistory->setOpenLinks(false);
    m_chatHistory->setFrameStyle(QFrame::NoFrame);
    layout->addWidget(m_chatHistory, 1);
    
    m_inputWidget = new AiChatInputWidget(this);
    layout->addWidget(m_inputWidget, 0);
    
    connect(m_inputWidget, &AiChatInputWidget::messageSubmitted, this, &AiChatWidget::sendMessage);
    connect(m_inputWidget, &AiChatInputWidget::newChatClicked, this, &AiChatWidget::clearChat);
    connect(m_inputWidget, &AiChatInputWidget::stopClicked, m_client, &LlamaClient::stopChat);
    connect(m_chatHistory, &QTextBrowser::anchorClicked, this, &AiChatWidget::onAnchorClicked);
    
    connect(m_client, &LlamaClient::chatTokenReceived, this, &AiChatWidget::onChatTokenReceived);
    connect(m_client, &LlamaClient::chatResponseFinished, this, &AiChatWidget::onChatFinished);
    connect(m_client, &LlamaClient::errorOccurred, this, &AiChatWidget::onError);
    connect(m_client, &LlamaClient::warningOccurred, this, &AiChatWidget::onWarning);
    
    // Display welcome guide
    clearChat();
}

// (eventFilter removed because AiChatInputWidget handles its own input keys now)
bool AiChatWidget::eventFilter(QObject *watched, QEvent *event)
{
    return QWidget::eventFilter(watched, event);
}

// ##Method purpose: Reads user input, builds the JSON message history, and starts the stream.
void AiChatWidget::sendMessage(const QString &text)
{
    if (text.isEmpty()) return;
    
    m_rawMarkdown += QStringLiteral("**You:**\n\n") + text + QStringLiteral("\n\n---\n\n**AI:**\n\n");
    renderMarkdown();
    
    // ##Condition purpose: Inject or update the system prompt on every message to keep file context fresh.
    KTextEditor::View* activeView = nullptr;
    auto core = KDevelop::ICore::self();
    auto activeDoc = core ? core->documentController()->activeDocument() : nullptr;
    if (activeDoc) {
        activeView = activeDoc->activeTextView();
    }
    QString sysPrompt = m_context->buildSystemPrompt(activeView);
    if (m_messageHistory.isEmpty()) {
        QJsonObject sysMsg;
        sysMsg[QStringLiteral("role")] = QStringLiteral("system");
        sysMsg[QStringLiteral("content")] = sysPrompt;
        m_messageHistory.append(sysMsg);
    } else {
        QJsonObject sysMsg = m_messageHistory.first().toObject();
        if (sysMsg[QStringLiteral("role")].toString() == QStringLiteral("system")) {
            sysMsg[QStringLiteral("content")] = sysPrompt;
            m_messageHistory.replace(0, sysMsg);
        }
    }
    
    QJsonObject userMsg;
    userMsg[QStringLiteral("role")] = QStringLiteral("user");
    userMsg[QStringLiteral("content")] = text;
    m_messageHistory.append(userMsg);
    
    m_currentAssistantResponse.clear();
    m_inputWidget->setPromptRunning(true);
    m_client->requestChat(m_messageHistory);
}

// ##Method purpose: Receives incoming streamed tokens and renders the evolving markdown.
void AiChatWidget::onChatTokenReceived(const QString &token)
{
    m_currentAssistantResponse += token;
    m_rawMarkdown += token;
    if (!m_renderTimer->isActive()) {
        m_renderTimer->start();
    }
}

// ##Method purpose: Triggered when the AI finishes responding, finalizing the message block.
void AiChatWidget::onChatFinished()
{
    if (m_renderTimer->isActive()) {
        m_renderTimer->stop();
    }

    m_inputWidget->setPromptRunning(false);
    
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
    m_inputWidget->setPromptRunning(false);
    m_rawMarkdown += QStringLiteral("\n\n**Error:** `") + error + QStringLiteral("`\n\n---\n\n");
    renderMarkdown();
}

// ##Method purpose: Displays a security warning in the chat log.
void AiChatWidget::onWarning(const QString &warning)
{
    m_rawMarkdown += QStringLiteral("\n\n**Security Warning:** ") + warning + QStringLiteral("\n\n---\n\n");
    renderMarkdown();
}

// ##Method purpose: Refreshes the display with Qt6's native Markdown parser.
void AiChatWidget::renderMarkdown()
{
    QTextDocument::MarkdownFeatures features = QTextDocument::MarkdownDialectGitHub;
    features |= QTextDocument::MarkdownNoHTML;
    m_chatHistory->document()->setMarkdown(m_rawMarkdown, features);
    m_chatHistory->moveCursor(QTextCursor::End);
}

// ##Method purpose: Clears the message history and resets the chat UI.
void AiChatWidget::clearChat()
{
    // Save history before clearing, if we have active user/assistant messages.
    if (m_messageHistory.size() > 1) { // > 1 to ignore empty chats with only the system prompt
        const QString dataPath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).filePath(QStringLiteral("chat_history"));
        QDir dir(dataPath);
        if (dir.mkpath(dataPath)) {
            const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd_HH-mm-ss"));
            const QString filename = dir.filePath(QStringLiteral("chat_") + timestamp + QStringLiteral(".json"));
            QFile file(filename);
            if (file.open(QIODevice::WriteOnly)) {
                const QJsonDocument doc(m_messageHistory);
                file.write(doc.toJson());
                file.close();
            }
        }
    }

    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_rawMarkdown = QStringLiteral("# KDev LLM\n\nWelcome to KDev LLM, your AI Assistant for KDevelop!\n\n## Features:\n- **Chat**: Type below and press `Enter` to ask questions about your code.\n- **Refactor**: Select code, right-click (or Tools menu) and choose **AI: Refactor Selection...**\n- **Autocomplete**: Press `Ctrl+Space` while typing to get AI code suggestions.\n\n*(Note: Ensure your local Llama.cpp server is running at the configured endpoint in Settings)*\n\n---\n\n");
    renderMarkdown();
}

// ##Method purpose: Securely handles clicked links to prevent arbitrary scheme execution.
void AiChatWidget::onAnchorClicked(const QUrl &url)
{
    // ##Condition purpose: Handle relative URLs for internal anchor navigation.
    if (url.isRelative()) {
        // ##Condition purpose: Ensure fragment is present before scrolling.
        if (!url.fragment().isEmpty()) {
            m_chatHistory->scrollToAnchor(url.fragment());
        }
        return;
    }

    const QString scheme = url.scheme();
    // ##Condition purpose: Restrict external link opening to safe HTTP/S schemes.
    if (scheme == QStringLiteral("http") || scheme == QStringLiteral("https")) {
        QDesktopServices::openUrl(url);
    }
}

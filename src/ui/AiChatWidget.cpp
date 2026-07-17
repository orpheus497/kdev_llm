// ##Script function and purpose: Implements the native chat UI with QListView, SQLite history, and @file context injection.
#include "AiChatWidget.h"
#include "AiChatInputWidget.h"
#include "ChatMessageModel.h"
#include "ChatMessageDelegate.h"
#include "../network/LlamaClient.h"
#include "../context/ContextManager.h"
#include "../storage/ChatDatabase.h"

#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListView>
#include <QComboBox>
#include <QPushButton>
#include <QScrollBar>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTimer>

#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>

// ##Method purpose: Sets up the native list-based layout, initializes components, and connects signals.
AiChatWidget::AiChatWidget(QWidget *parent)
    : QWidget(parent)
    , m_client(new LlamaClient(this))
    , m_context(new ContextManager(this))
    , m_database(new ChatDatabase(this))
    , m_messageModel(new ChatMessageModel(this))
    , m_delegate(new ChatMessageDelegate(this))
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    // ##Step purpose: Create the conversation history selector toolbar.
    auto *toolbar = new QHBoxLayout();
    toolbar->setContentsMargins(4, 4, 4, 4);

    m_conversationSelector = new QComboBox(this);
    m_conversationSelector->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_conversationSelector->setToolTip(QStringLiteral("Select a previous conversation"));
    toolbar->addWidget(m_conversationSelector);

    auto *newChatBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-new")), QString(), this);
    newChatBtn->setToolTip(QStringLiteral("Start a new conversation"));
    newChatBtn->setFlat(true);
    toolbar->addWidget(newChatBtn);

    layout->addLayout(toolbar);

    // ##Step purpose: Create the native QListView for chat messages.
    m_chatView = new QListView(this);
    m_chatView->setModel(m_messageModel);
    m_chatView->setItemDelegate(m_delegate);
    m_chatView->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    m_chatView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatView->setSelectionMode(QAbstractItemView::NoSelection);
    m_chatView->setFrameStyle(QFrame::NoFrame);
    m_chatView->setSpacing(2);
    m_chatView->setWordWrap(true);
    m_chatView->setResizeMode(QListView::Adjust);
    m_chatView->setUniformItemSizes(false);
    layout->addWidget(m_chatView, 1);
    
    // ##Step purpose: Create the input area at the bottom.
    m_inputWidget = new AiChatInputWidget(this);
    layout->addWidget(m_inputWidget, 0);

    // ##Step purpose: Populate @file autocompletion from the project.
    QStringList projectFiles = m_context->getProjectFiles();
    m_inputWidget->setAvailableFiles(projectFiles);

    // ##Step purpose: Connect UI signals — both the toolbar button and the input widget's New Chat button.
    connect(m_inputWidget, &AiChatInputWidget::messageSubmitted, this, &AiChatWidget::sendMessage);
    connect(m_inputWidget, &AiChatInputWidget::stopClicked, m_client, &LlamaClient::stopChat);
    connect(m_inputWidget, &AiChatInputWidget::newChatClicked, this, &AiChatWidget::clearChat);
    connect(newChatBtn, &QPushButton::clicked, this, &AiChatWidget::clearChat);
    connect(m_conversationSelector, QOverload<int>::of(&QComboBox::activated), this, &AiChatWidget::loadConversation);
    
    // ##Step purpose: Connect network signals.
    connect(m_client, &LlamaClient::chatTokenReceived, this, &AiChatWidget::onChatTokenReceived);
    connect(m_client, &LlamaClient::chatResponseFinished, this, &AiChatWidget::onChatFinished);
    connect(m_client, &LlamaClient::errorOccurred, this, &AiChatWidget::onError);
    connect(m_client, &LlamaClient::warningOccurred, this, &AiChatWidget::onWarning);

    // ##Step purpose: Ensure the view auto-scrolls when model content changes.
    connect(m_messageModel, &QAbstractItemModel::rowsInserted, this, [this]() {
        // ##Step purpose: Defer scroll to after the layout has updated.
        QTimer::singleShot(0, this, &AiChatWidget::scrollToBottom);
    });
    connect(m_messageModel, &QAbstractItemModel::dataChanged, this, [this]() {
        QTimer::singleShot(0, this, &AiChatWidget::scrollToBottom);
    });
    
    // ##Step purpose: Start with a fresh conversation.
    refreshConversationList();
    clearChat();
}

// ##Method purpose: Extracts @file references and resolves them to context using DUChain.
QString AiChatWidget::resolveFileReferences(const QString &text) const
{
    static const QRegularExpression fileRefRe(QStringLiteral("@(\\S+)"));
    QString contextBlock;

    auto it = fileRefRe.globalMatch(text);
    // ##Loop purpose: Find all @file references in the user message.
    while (it.hasNext()) {
        auto match = it.next();
        QString filePath = match.captured(1);
        QString fileContext = m_context->extractRelevantFileContext(filePath);
        if (!fileContext.isEmpty()) {
            contextBlock += QStringLiteral("\n\n--- Referenced File Context ---\n") + fileContext;
        }
    }
    return contextBlock;
}

// ##Method purpose: Reads user input, builds context, saves to SQLite, and starts the LLM stream.
void AiChatWidget::sendMessage(const QString &text)
{
    if (text.isEmpty()) return;

    // ##Step purpose: Create a new conversation in SQLite if this is the first message.
    if (m_currentConversationId < 0) {
        // ##Step purpose: Use the first 40 chars of the user message as the conversation title.
        QString title = text.left(40);
        if (text.length() > 40) title += QStringLiteral("...");
        m_currentConversationId = m_database->createConversation(title);
        refreshConversationList();
    }

    // ##Step purpose: Display the user message in the native list view.
    m_messageModel->addMessage(QStringLiteral("user"), text);
    m_database->addMessage(m_currentConversationId, QStringLiteral("user"), text);

    // ##Step purpose: Show a "thinking" indicator so the user knows the system is waiting for the LLM.
    m_messageModel->addMessage(QStringLiteral("thinking"), QStringLiteral("Waiting for response..."));

    // ##Step purpose: Inject or update the system prompt on every message to keep file context fresh.
    KTextEditor::View* activeView = nullptr;
    auto core = KDevelop::ICore::self();
    auto activeDoc = core ? core->documentController()->activeDocument() : nullptr;
    if (activeDoc) {
        activeView = activeDoc->activeTextView();
    }
    QString sysPrompt = m_context->buildSystemPrompt(activeView);

    // ##Step purpose: Resolve any @file references and append their context to the system prompt.
    QString fileContext = resolveFileReferences(text);
    if (!fileContext.isEmpty()) {
        sysPrompt += fileContext;
    }

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

// ##Method purpose: Receives streaming tokens and updates the native list model incrementally.
// The first token automatically replaces the "thinking" placeholder.
void AiChatWidget::onChatTokenReceived(const QString &token)
{
    m_currentAssistantResponse += token;
    m_messageModel->appendToLastAssistant(token);
}

// ##Method purpose: Finalises the assistant message and persists it to SQLite.
void AiChatWidget::onChatFinished()
{
    m_inputWidget->setPromptRunning(false);
    m_messageModel->finaliseLastAssistant();
    
    // ##Condition purpose: Only persist and track non-empty responses.
    if (!m_currentAssistantResponse.isEmpty()) {
        QJsonObject assistantMsg;
        assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
        assistantMsg[QStringLiteral("content")] = m_currentAssistantResponse;
        m_messageHistory.append(assistantMsg);

        // ##Step purpose: Persist the assistant's response to SQLite.
        if (m_currentConversationId >= 0) {
            m_database->addMessage(m_currentConversationId, QStringLiteral("assistant"), m_currentAssistantResponse);
        }
    } else {
        // ##Step purpose: If no tokens were received, remove the thinking indicator.
        m_messageModel->removeThinkingIndicator();
    }

    refreshConversationList();
}

// ##Method purpose: Displays a network or parsing error in the chat as a native error bubble.
void AiChatWidget::onError(const QString &error)
{
    m_inputWidget->setPromptRunning(false);
    // ##Step purpose: Remove the thinking indicator before showing the error.
    m_messageModel->removeThinkingIndicator();
    m_messageModel->addMessage(QStringLiteral("error"), error);

    if (m_currentConversationId >= 0) {
        m_database->addMessage(m_currentConversationId, QStringLiteral("error"), error);
    }
}

// ##Method purpose: Displays a security warning as a native warning bubble.
void AiChatWidget::onWarning(const QString &warning)
{
    m_messageModel->addMessage(QStringLiteral("warning"), warning);
}

// ##Method purpose: Starts a new conversation, preserving the current one in SQLite.
void AiChatWidget::clearChat()
{
    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_currentConversationId = -1;
    m_messageModel->clear();

    // ##Step purpose: Show the welcome message as a system-role entry.
    m_messageModel->addMessage(QStringLiteral("welcome"),
        QStringLiteral("Welcome to KDev LLM, your AI Assistant for KDevelop!\n\n"
                       "• Chat: Type below and press Enter to ask questions about your code.\n"
                       "• @file: Reference project files for context (e.g. @src/main.cpp).\n"
                       "• Refactor: Select code → right-click → AI: Refactor Selection...\n"
                       "• Autocomplete: Press Ctrl+Space for AI code suggestions.\n\n"
                       "Ensure your LLM server is running at the configured endpoint (Settings → KDev LLM)."));

    refreshConversationList();
    // ##Step purpose: Reset the combo box to the first item (placeholder) for a new chat.
    m_conversationSelector->setCurrentIndex(0);
}

// ##Method purpose: Loads a conversation from SQLite by its combo box index.
void AiChatWidget::loadConversation(int comboIndex)
{
    qint64 convId = m_conversationSelector->itemData(comboIndex).toLongLong();
    // ##Condition purpose: Ignore the placeholder "— Conversation History —" entry.
    if (convId <= 0) return;

    m_currentConversationId = convId;
    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_messageModel->clear();

    // ##Step purpose: Reload messages from SQLite and repopulate the model and JSON history.
    auto messages = m_database->getMessages(convId);
    // ##Loop purpose: Rebuild the visual model and the JSON history from database records.
    for (const auto &msg : messages) {
        // ##Condition purpose: Skip system messages from display but keep them in JSON history.
        if (msg.role == QStringLiteral("system")) {
            QJsonObject sysMsg;
            sysMsg[QStringLiteral("role")] = msg.role;
            sysMsg[QStringLiteral("content")] = msg.content;
            m_messageHistory.append(sysMsg);
            continue;
        }

        // ##Condition purpose: Skip non-displayable roles like "thinking".
        if (msg.role == QStringLiteral("thinking")) {
            continue;
        }

        m_messageModel->addMessage(msg.role, msg.content);

        // ##Condition purpose: Only add user/assistant messages to the JSON history sent to the LLM.
        if (msg.role != QStringLiteral("error") && msg.role != QStringLiteral("warning")) {
            QJsonObject jsonMsg;
            jsonMsg[QStringLiteral("role")] = msg.role;
            jsonMsg[QStringLiteral("content")] = msg.content;
            m_messageHistory.append(jsonMsg);
        }
    }

    // ##Step purpose: Keep the combo box highlighting the currently loaded conversation.
    m_conversationSelector->setCurrentIndex(comboIndex);
}

// ##Method purpose: Scrolls the QListView to show the most recent message.
void AiChatWidget::scrollToBottom()
{
    QScrollBar *sb = m_chatView->verticalScrollBar();
    if (sb) {
        sb->setValue(sb->maximum());
    }
}

// ##Method purpose: Refreshes the conversation combo box from the SQLite database.
void AiChatWidget::refreshConversationList()
{
    // ##Step purpose: Remember the current selection to preserve it after repopulating.
    qint64 currentId = m_currentConversationId;
    int selectIndex = 0;

    m_conversationSelector->blockSignals(true);
    m_conversationSelector->clear();
    m_conversationSelector->addItem(QStringLiteral("— Conversation History —"), QVariant(static_cast<qint64>(0)));

    auto conversations = m_database->getConversations();
    int idx = 1; // start at 1 because index 0 is the placeholder
    // ##Loop purpose: Add each conversation to the selector with its title and ID.
    for (const auto &conv : conversations) {
        QString label = conv.title;
        if (label.isEmpty()) {
            label = QStringLiteral("Chat %1").arg(conv.id);
        }
        if (conv.messageCount > 0) {
            label += QStringLiteral(" (%1 msgs)").arg(conv.messageCount);
        }
        m_conversationSelector->addItem(label, QVariant(conv.id));

        // ##Condition purpose: Track which index matches the currently active conversation.
        if (conv.id == currentId) {
            selectIndex = idx;
        }
        ++idx;
    }

    // ##Step purpose: Restore the selection to the currently active conversation.
    m_conversationSelector->setCurrentIndex(selectIndex);
    m_conversationSelector->blockSignals(false);
}

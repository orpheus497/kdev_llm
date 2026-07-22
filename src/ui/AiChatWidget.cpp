// ##Script function and purpose: Implements the native chat UI with QListView, SQLite history, @file context, and conversation management.
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
#include <QSet>
#include <QTimer>
#include <QDir>
#include <KLocalizedString>
#include <QFileInfo>
#include <QMessageBox>

#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <util/path.h>

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

    // ##Action purpose: Create a 'New Chat' button.
    auto *newChatBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("document-new")), i18n("New"), this);
    newChatBtn->setToolTip(QStringLiteral("Start a new conversation"));
    toolbar->addWidget(newChatBtn);

    // ##Action purpose: Create a 'Delete Chat' button.
    m_deleteBtn = new QPushButton(QIcon::fromTheme(QStringLiteral("edit-delete")), i18n("Delete"), this);
    m_deleteBtn->setToolTip(QStringLiteral("Delete the selected conversation"));
    m_deleteBtn->setEnabled(false);
    toolbar->addWidget(m_deleteBtn);

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
    connect(m_deleteBtn, &QPushButton::clicked, this, &AiChatWidget::deleteCurrentConversation);
    connect(m_conversationSelector, QOverload<int>::of(&QComboBox::activated), this, &AiChatWidget::loadConversation);

    // ##Step purpose: Enable/disable delete button based on combo selection.
    connect(m_conversationSelector, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int idx) {
        qint64 convId = m_conversationSelector->itemData(idx).toLongLong();
        m_deleteBtn->setEnabled(convId > 0);
    });
    
    // ##Step purpose: Connect network signals.
    connect(m_client, &LlamaClient::chatTokenReceived, this, &AiChatWidget::onChatTokenReceived);
    connect(m_client, &LlamaClient::chatResponseFinished, this, &AiChatWidget::onChatFinished);
    connect(m_client, &LlamaClient::errorOccurred, this, &AiChatWidget::onError);
    connect(m_client, &LlamaClient::warningOccurred, this, &AiChatWidget::onWarning);

    // ##Step purpose: Ensure the view auto-scrolls when model content changes.
    connect(m_messageModel, &QAbstractItemModel::rowsInserted, this, [this]() {
        QTimer::singleShot(0, this, &AiChatWidget::scrollToBottom);
    });
    connect(m_messageModel, &QAbstractItemModel::dataChanged, this, [this]() {
        QTimer::singleShot(0, this, &AiChatWidget::scrollToBottom);
    });
    
    // ##Step purpose: Start with a fresh conversation.
    refreshConversationList();
    clearChat();
}

// ##Method purpose: Resolves a relative file path against the current project root.
QString AiChatWidget::resolveFilePath(const QString &relativePath) const
{
    // ##Condition purpose: If the path is already absolute and exists, use it directly.
    if (QFileInfo(relativePath).isAbsolute()) {
        if (QFileInfo::exists(relativePath)) {
            return relativePath;
        }
        return QString();
    }

    // ##Step purpose: Find the project root to resolve relative paths.
    auto *core = KDevelop::ICore::self();
    if (!core || !core->projectController()) return QString();

    QString projectRoot;
    auto projects = core->projectController()->projects();
    // ##Loop purpose: Try each open project to find one that contains the referenced file.
    for (auto *project : projects) {
        QString root = project->path().toLocalFile();
        QString candidate = QDir(root).filePath(relativePath);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
        // ##Step purpose: Remember the first project root as a fallback.
        if (projectRoot.isEmpty()) {
            projectRoot = root;
        }
    }

    // ##Step purpose: Try to resolve against the active document's directory.
    auto activeDoc = core->documentController()->activeDocument();
    if (activeDoc && activeDoc->textDocument()) {
        QString docDir = QFileInfo(activeDoc->textDocument()->url().toLocalFile()).absolutePath();
        QString candidate = QDir(docDir).filePath(relativePath);
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    // ##Step purpose: Last resort — try the first project root even if the file doesn't exist.
    if (!projectRoot.isEmpty()) {
        return QDir(projectRoot).filePath(relativePath);
    }

    return QString();
}

// ##Method purpose: Extracts @file references and resolves them to context using DUChain.
QString AiChatWidget::resolveFileReferences(const QString &text) const
{
    static const QRegularExpression fileRefRe(QStringLiteral("@(\\S+)"));
    QString contextBlock;
    QSet<QString> processedPaths;

    auto it = fileRefRe.globalMatch(text);
    // ##Loop purpose: Find all @file references in the user message.
    while (it.hasNext()) {
        auto match = it.next();
        QString rawPath = match.captured(1);

        // ##Step purpose: Resolve the path against the project root before extracting context.
        QString resolvedPath = resolveFilePath(rawPath);
        // ##Action purpose: Generate a canonical path key for safe duplication checks.
        QString normalizedKey = QFileInfo(resolvedPath).canonicalFilePath();
        // ##Condition purpose: Fallback to a cleaned resolved path if canonicalization fails.
        if (normalizedKey.isEmpty()) normalizedKey = QDir::cleanPath(resolvedPath);

        // ##Condition purpose: Prevent duplication of already extracted file context.
        if (resolvedPath.isEmpty() || processedPaths.contains(normalizedKey)) {
            continue;
        }
        processedPaths.insert(normalizedKey);

        QString fileContext = m_context->extractRelevantFileContext(resolvedPath);
        if (!fileContext.isEmpty()) {
            contextBlock += QStringLiteral("\n\n--- Referenced File Context (@") + rawPath + QStringLiteral(") ---\n") + fileContext;
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

    // ##Step purpose: Build the system prompt with fresh file context.
    KTextEditor::View* activeView = nullptr;
    auto core = KDevelop::ICore::self();
    auto activeDoc = core ? core->documentController()->activeDocument() : nullptr;
    if (activeDoc) {
        activeView = activeDoc->activeTextView();
    }
    QString sysPrompt = m_context->buildSystemPrompt(activeView);

    // ##Step purpose: Resolve any @file references from the entire conversation and append their context to the system prompt.
    QString allTextForRefs = text;
    // ##Loop purpose: Accumulate user messages to correctly resolve all historical file context.
    for (const auto &msg : m_messageHistory) {
        QJsonObject obj = msg.toObject();
        // ##Condition purpose: Extract only user message content for file references.
        if (obj[QStringLiteral("role")].toString() == QStringLiteral("user")) {
            allTextForRefs += QStringLiteral(" ") + obj[QStringLiteral("content")].toString();
        }
    }
    QString fileContext = resolveFileReferences(allTextForRefs);
    if (!fileContext.isEmpty()) {
        sysPrompt += fileContext;
    }

    // ##Step purpose: Ensure the system prompt is always at index 0 of the message history.
    // This handles both fresh conversations and loaded conversations that may lack a system entry.
    QJsonObject sysMsg;
    sysMsg[QStringLiteral("role")] = QStringLiteral("system");
    sysMsg[QStringLiteral("content")] = sysPrompt;

    if (m_messageHistory.isEmpty()) {
        m_messageHistory.append(sysMsg);
    } else {
        QJsonObject first = m_messageHistory.first().toObject();
        if (first[QStringLiteral("role")].toString() == QStringLiteral("system")) {
            m_messageHistory.replace(0, sysMsg);
        } else {
            // ##Step purpose: Insert system prompt at position 0 if the loaded history didn't have one.
            QJsonArray updated;
            updated.append(sysMsg);
            for (int i = 0; i < m_messageHistory.size(); ++i) {
                updated.append(m_messageHistory.at(i));
            }
            m_messageHistory = updated;
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
    
    if (!m_currentAssistantResponse.isEmpty()) {
        QJsonObject assistantMsg;
        assistantMsg[QStringLiteral("role")] = QStringLiteral("assistant");
        assistantMsg[QStringLiteral("content")] = m_currentAssistantResponse;
        m_messageHistory.append(assistantMsg);

        if (m_currentConversationId >= 0) {
            m_database->addMessage(m_currentConversationId, QStringLiteral("assistant"), m_currentAssistantResponse);
        }
    } else {
        m_messageModel->removeThinkingIndicator();
    }

    refreshConversationList();
}

// ##Method purpose: Displays a network or parsing error in the chat as a native error bubble.
void AiChatWidget::onError(const QString &error)
{
    m_inputWidget->setPromptRunning(false);
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

// ##Method purpose: Starts a new conversation.
void AiChatWidget::clearChat()
{
    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_currentConversationId = -1;
    m_messageModel->clear();

    m_messageModel->addMessage(QStringLiteral("welcome"),
        QStringLiteral("Welcome to KDev LLM, your AI Assistant for KDevelop!\n\n"
                       "• Chat: Type below and press Enter to ask questions about your code.\n"
                       "• @file: Reference project files for context (e.g. @src/main.cpp).\n"
                       "• Refactor: Select code → right-click → AI: Refactor Selection...\n"
                       "• Autocomplete: Press Ctrl+Space for AI code suggestions.\n\n"
                       "Ensure your LLM server is running at the configured endpoint (Settings → KDev LLM)."));

    refreshConversationList();
    m_conversationSelector->setCurrentIndex(0);
}

// ##Method purpose: Loads a conversation from SQLite by its combo box index.
void AiChatWidget::loadConversation(int comboIndex)
{
    qint64 convId = m_conversationSelector->itemData(comboIndex).toLongLong();
    if (convId <= 0) return;

    m_currentConversationId = convId;
    m_messageHistory = QJsonArray();
    m_currentAssistantResponse.clear();
    m_messageModel->clear();

    auto messages = m_database->getMessages(convId);
    // ##Loop purpose: Rebuild the visual model and the JSON history from database records.
    for (const auto &msg : messages) {
        // ##Condition purpose: Skip system and thinking messages from display.
        if (msg.role == QStringLiteral("system") || msg.role == QStringLiteral("thinking")) {
            // ##Condition purpose: Keep system messages in JSON history for continuity.
            if (msg.role == QStringLiteral("system")) {
                QJsonObject sysMsg;
                sysMsg[QStringLiteral("role")] = msg.role;
                sysMsg[QStringLiteral("content")] = msg.content;
                m_messageHistory.append(sysMsg);
            }
            continue;
        }

        m_messageModel->addMessage(msg.role, msg.content);

        // ##Condition purpose: Only user/assistant messages go to the LLM JSON history.
        if (msg.role == QStringLiteral("user") || msg.role == QStringLiteral("assistant")) {
            QJsonObject jsonMsg;
            jsonMsg[QStringLiteral("role")] = msg.role;
            jsonMsg[QStringLiteral("content")] = msg.content;
            m_messageHistory.append(jsonMsg);
        }
    }

    m_conversationSelector->setCurrentIndex(comboIndex);
}

// ##Method purpose: Prompts the user to confirm conversation deletion.
QMessageBox::StandardButton AiChatWidget::askDeletionConfirmation()
{
    // ##Step purpose: Display standard question box for deletion confirmation.
    // ##Action purpose: Prompt user via QMessageBox::question dialog with QMessageBox::No as default.
    return QMessageBox::question(this, i18n("Confirm Delete"),
                                  i18n("Are you sure you want to delete this conversation?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No);
}

// ##Method purpose: Deletes the conversation selected in the combo box from SQLite.
void AiChatWidget::deleteCurrentConversation()
{
    int idx = m_conversationSelector->currentIndex();
    qint64 convId = m_conversationSelector->itemData(idx).toLongLong();
    // ##Condition purpose: Ignore the placeholder entry.
    if (convId <= 0) return;

    // ##Step purpose: Request user confirmation before deletion.
    QMessageBox::StandardButton reply = askDeletionConfirmation();
    // ##Condition purpose: Abort deletion if confirmation was not approved.
    if (reply != QMessageBox::Yes) {
        return;
    }

    m_database->deleteConversation(convId);

    // ##Condition purpose: If the deleted conversation is the currently active one, reset to a new chat.
    if (convId == m_currentConversationId) {
        clearChat();
    } else {
        refreshConversationList();
    }
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
    qint64 currentId = m_currentConversationId;
    int selectIndex = 0;

    m_conversationSelector->blockSignals(true);
    m_conversationSelector->clear();
    m_conversationSelector->addItem(QStringLiteral("— Conversation History —"), QVariant(static_cast<qint64>(0)));

    auto conversations = m_database->getConversations();
    int idx = 1;
    for (const auto &conv : conversations) {
        QString label = conv.title;
        if (label.isEmpty()) {
            label = QStringLiteral("Chat %1").arg(conv.id);
        }
        if (conv.messageCount > 0) {
            label += QStringLiteral(" (%1 msgs)").arg(conv.messageCount);
        }
        m_conversationSelector->addItem(label, QVariant(conv.id));

        if (conv.id == currentId) {
            selectIndex = idx;
        }
        ++idx;
    }

    m_conversationSelector->setCurrentIndex(selectIndex);
    m_conversationSelector->blockSignals(false);

    // ##Step purpose: Update delete button state after refresh.
    qint64 selectedId = m_conversationSelector->currentData().toLongLong();
    m_deleteBtn->setEnabled(selectedId > 0);
}

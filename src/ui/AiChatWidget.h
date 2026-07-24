// ##Script function and purpose: Defines the UI widget for the AI chat panel with native Qt list-based message display.
#pragma once

#include <QWidget>
#include <QJsonArray>
#include <QUrl>
#include <QMessageBox>

class QListView;
class QComboBox;
class QPushButton;
class AiChatInputWidget;
class LlamaClient;
class ContextManager;
class ChatMessageModel;
class ChatMessageDelegate;
class ChatDatabase;

// ##Class purpose: Manages the native chat display, input, history persistence, and LLM streaming for the AI panel.
class AiChatWidget : public QWidget {
    Q_OBJECT
    friend class TestAiChatWidget;
public:
    // ##Method purpose: Constructor for the chat widget.
    explicit AiChatWidget(QWidget *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~AiChatWidget() override = default;

private Q_SLOTS:
    // ##Method purpose: Triggered when the user clicks Send; packages the message.
    void sendMessage(const QString &text);
    
    // ##Method purpose: Triggered when a new token arrives from the SSE stream.
    void onChatTokenReceived(const QString &token);
    
    // ##Method purpose: Triggered when the SSE stream completes.
    void onChatFinished();
    
    // ##Method purpose: Triggered when the LlamaClient encounters an error.
    void onError(const QString &error);

    // ##Method purpose: Triggered when the LlamaClient issues a security warning.
    void onWarning(const QString &warning);

    // ##Method purpose: Starts a new conversation, saving the current one to SQLite.
    void clearChat();

    // ##Method purpose: Loads a previous conversation from the history selector.
    void loadConversation(int comboIndex);

    // ##Method purpose: Deletes the currently selected conversation from SQLite.
    void deleteCurrentConversation();

protected:
    // ##Method purpose: Prompts the user to confirm conversation deletion.
    virtual QMessageBox::StandardButton askDeletionConfirmation();

private:
    // ##Method purpose: Scrolls the list view to the bottom to show the latest message.
    void scrollToBottom();

    // ##Method purpose: Populates the conversation selector combo box from the database.
    void refreshConversationList();

    // ##Method purpose: Extracts @file references from user text and builds context string.
    QString resolveFileReferences(const QString &text) const;

    // ##Method purpose: Resolves a relative file path against the project root.
    QString resolveFilePath(const QString &relativePath) const;

    LlamaClient *m_client;
    ContextManager *m_context;
    ChatDatabase *m_database;
    ChatMessageModel *m_messageModel;
    ChatMessageDelegate *m_delegate;

    QListView *m_chatView;
    QComboBox *m_conversationSelector;
    QPushButton *m_deleteBtn;
    AiChatInputWidget *m_inputWidget;

    qint64 m_currentConversationId = -1;
    QJsonArray m_messageHistory;
    QString m_currentAssistantResponse;
};

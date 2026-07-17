// ##Script function and purpose: Defines an SQLite-backed storage engine for chat conversation history.
#pragma once

#include <QObject>
#include <QString>
#include <QJsonArray>
#include <QDateTime>
#include <QSqlDatabase>

// ##Class purpose: Stores and retrieves chat conversations using SQLite for persistent history management.
class ChatDatabase : public QObject {
    Q_OBJECT
public:
    // ##Class purpose: Represents a single conversation summary for the history browser.
    struct ConversationSummary {
        qint64 id;
        QString title;
        QDateTime createdAt;
        QDateTime updatedAt;
        int messageCount;
    };

    // ##Class purpose: Represents a single message within a conversation.
    struct ChatMessage {
        qint64 id;
        qint64 conversationId;
        QString role;      // "system", "user", "assistant", "error"
        QString content;
        QDateTime timestamp;
    };

    // ##Method purpose: Constructor; opens or creates the SQLite database.
    explicit ChatDatabase(QObject *parent = nullptr);
    // ##Method purpose: Destructor.
    ~ChatDatabase() override;

    // ##Method purpose: Creates a new conversation and returns its ID.
    qint64 createConversation(const QString &title = QString());

    // ##Method purpose: Appends a message to a conversation.
    void addMessage(qint64 conversationId, const QString &role, const QString &content);

    // ##Method purpose: Updates the title of an existing conversation.
    void updateConversationTitle(qint64 conversationId, const QString &title);

    // ##Method purpose: Retrieves all messages for a given conversation.
    QList<ChatMessage> getMessages(qint64 conversationId) const;

    // ##Method purpose: Retrieves all conversation summaries, most recent first.
    QList<ConversationSummary> getConversations() const;

    // ##Method purpose: Deletes a conversation and all its messages.
    void deleteConversation(qint64 conversationId);

    // ##Method purpose: Converts conversation messages to the JSON format expected by the LLM API.
    QJsonArray messagesToJson(qint64 conversationId) const;

private:
    // ##Method purpose: Creates the database tables if they don't exist.
    void initDatabase();

    QSqlDatabase m_db;
    QString m_connectionName;
};

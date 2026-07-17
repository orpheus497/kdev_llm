// ##Script function and purpose: Implements SQLite-backed chat conversation storage and retrieval.
#include "ChatDatabase.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QStandardPaths>
#include <QDir>
#include <QJsonObject>
#include <QDebug>
#include <QUuid>

// ##Method purpose: Opens or creates the SQLite database in the KDevelop data directory.
ChatDatabase::ChatDatabase(QObject *parent)
    : QObject(parent)
{
    // ##Step purpose: Generate a unique connection name to avoid conflicts with other QSqlDatabase users.
    m_connectionName = QStringLiteral("kdevllm_chat_") + QUuid::createUuid().toString(QUuid::Id128).left(8);

    const QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataPath);
    dir.mkpath(QStringLiteral("kdevllm"));

    const QString dbPath = dir.filePath(QStringLiteral("kdevllm/chat_history.db"));

    m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_db.setDatabaseName(dbPath);

    // ##Condition purpose: Log an error if the database cannot be opened.
    if (!m_db.open()) {
        qWarning() << "KDev LLM: Failed to open chat database:" << m_db.lastError().text();
        return;
    }

    initDatabase();
}

// ##Method purpose: Closes the database connection.
ChatDatabase::~ChatDatabase()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    QSqlDatabase::removeDatabase(m_connectionName);
}

// ##Method purpose: Creates the conversations and messages tables.
void ChatDatabase::initDatabase()
{
    QSqlQuery query(m_db);

    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS conversations ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  title TEXT NOT NULL DEFAULT '',"
        "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP"
        ")"
    ));

    query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  conversation_id INTEGER NOT NULL,"
        "  role TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,"
        "  FOREIGN KEY (conversation_id) REFERENCES conversations(id) ON DELETE CASCADE"
        ")"
    ));

    // ##Step purpose: Enable foreign key enforcement for CASCADE deletes.
    query.exec(QStringLiteral("PRAGMA foreign_keys = ON"));
}

// ##Method purpose: Inserts a new conversation row and returns the auto-generated ID.
qint64 ChatDatabase::createConversation(const QString &title)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("INSERT INTO conversations (title) VALUES (:title)"));
    query.bindValue(QStringLiteral(":title"), title.isEmpty() ? QStringLiteral("New Chat") : title);

    // ##Condition purpose: Return -1 on failure.
    if (!query.exec()) {
        qWarning() << "KDev LLM: Failed to create conversation:" << query.lastError().text();
        return -1;
    }
    return query.lastInsertId().toLongLong();
}

// ##Method purpose: Inserts a message into the specified conversation.
void ChatDatabase::addMessage(qint64 conversationId, const QString &role, const QString &content)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "INSERT INTO messages (conversation_id, role, content) VALUES (:cid, :role, :content)"
    ));
    query.bindValue(QStringLiteral(":cid"), conversationId);
    query.bindValue(QStringLiteral(":role"), role);
    query.bindValue(QStringLiteral(":content"), content);

    if (!query.exec()) {
        qWarning() << "KDev LLM: Failed to add message:" << query.lastError().text();
    }

    // ##Step purpose: Update the conversation's updated_at timestamp.
    QSqlQuery updateQuery(m_db);
    updateQuery.prepare(QStringLiteral("UPDATE conversations SET updated_at = CURRENT_TIMESTAMP WHERE id = :cid"));
    updateQuery.bindValue(QStringLiteral(":cid"), conversationId);
    updateQuery.exec();
}

// ##Method purpose: Changes the conversation title.
void ChatDatabase::updateConversationTitle(qint64 conversationId, const QString &title)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("UPDATE conversations SET title = :title WHERE id = :cid"));
    query.bindValue(QStringLiteral(":title"), title);
    query.bindValue(QStringLiteral(":cid"), conversationId);
    query.exec();
}

// ##Method purpose: Loads all messages for a conversation, ordered chronologically.
QList<ChatDatabase::ChatMessage> ChatDatabase::getMessages(qint64 conversationId) const
{
    QList<ChatMessage> result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT id, conversation_id, role, content, timestamp FROM messages "
        "WHERE conversation_id = :cid ORDER BY id ASC"
    ));
    query.bindValue(QStringLiteral(":cid"), conversationId);

    // ##Condition purpose: Process rows only if query succeeds.
    if (query.exec()) {
        // ##Loop purpose: Iterate over result rows and build ChatMessage objects.
        while (query.next()) {
            ChatMessage msg;
            msg.id = query.value(0).toLongLong();
            msg.conversationId = query.value(1).toLongLong();
            msg.role = query.value(2).toString();
            msg.content = query.value(3).toString();
            msg.timestamp = query.value(4).toDateTime();
            result.append(msg);
        }
    }
    return result;
}

// ##Method purpose: Retrieves summary information for all conversations.
QList<ChatDatabase::ConversationSummary> ChatDatabase::getConversations() const
{
    QList<ConversationSummary> result;
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral(
        "SELECT c.id, c.title, c.created_at, c.updated_at, COUNT(m.id) "
        "FROM conversations c LEFT JOIN messages m ON c.id = m.conversation_id "
        "GROUP BY c.id ORDER BY c.updated_at DESC"
    ));

    if (query.exec()) {
        while (query.next()) {
            ConversationSummary s;
            s.id = query.value(0).toLongLong();
            s.title = query.value(1).toString();
            s.createdAt = query.value(2).toDateTime();
            s.updatedAt = query.value(3).toDateTime();
            s.messageCount = query.value(4).toInt();
            result.append(s);
        }
    }
    return result;
}

// ##Method purpose: Removes a conversation and its messages from the database.
void ChatDatabase::deleteConversation(qint64 conversationId)
{
    QSqlQuery query(m_db);
    query.prepare(QStringLiteral("DELETE FROM conversations WHERE id = :cid"));
    query.bindValue(QStringLiteral(":cid"), conversationId);
    query.exec();
}

// ##Method purpose: Converts stored messages to the JSON array format for the LLM chat API.
QJsonArray ChatDatabase::messagesToJson(qint64 conversationId) const
{
    QJsonArray messages;
    const auto dbMessages = getMessages(conversationId);

    // ##Loop purpose: Convert each database row to a JSON object.
    for (const auto &msg : dbMessages) {
        // ##Condition purpose: Skip error entries — they are UI-only, not sent to the LLM.
        if (msg.role == QStringLiteral("error")) continue;

        QJsonObject obj;
        obj[QStringLiteral("role")] = msg.role;
        obj[QStringLiteral("content")] = msg.content;
        messages.append(obj);
    }
    return messages;
}

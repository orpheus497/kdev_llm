// ##Script function and purpose: Defines the data model for chat messages displayed in the QListView.
#pragma once

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

// ##Class purpose: Stores chat messages as a list model for the native QListView-based chat display.
class ChatMessageModel : public QAbstractListModel {
    Q_OBJECT
public:
    // ##Class purpose: Holds all data for a single chat message entry.
    struct Message {
        QString role;       // "user", "assistant", "system", "error", "warning", "welcome", "thinking"
        QString content;
        QDateTime timestamp;
        bool isStreaming = false;
    };

    enum Roles {
        RoleRole = Qt::UserRole + 1,
        ContentRole,
        TimestampRole,
        IsStreamingRole
    };

    // ##Method purpose: Constructor.
    explicit ChatMessageModel(QObject *parent = nullptr);

    // ##Method purpose: Returns the number of messages in the model.
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;

    // ##Method purpose: Returns message data for the requested role.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    // ##Method purpose: Appends a new message and notifies views.
    void addMessage(const QString &role, const QString &content);

    // ##Method purpose: Appends a streaming token to the last assistant message, creating one if needed.
    // If the last message is a "thinking" placeholder, it is replaced by the first token.
    void appendToLastAssistant(const QString &token);

    // ##Method purpose: Marks the last streaming message as complete.
    void finaliseLastAssistant();

    // ##Method purpose: Returns the content of the last assistant message.
    QString lastAssistantContent() const;

    // ##Method purpose: Removes the trailing "thinking" placeholder if present (e.g. on error).
    void removeThinkingIndicator();

    // ##Method purpose: Clears all messages from the model.
    void clear();

    // ##Method purpose: Returns a const reference to all messages for serialisation.
    const QVector<Message>& messages() const;

private:
    QVector<Message> m_messages;
};

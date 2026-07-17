// ##Script function and purpose: Implements the chat message list model with streaming and thinking indicator support.
#include "ChatMessageModel.h"

// ##Method purpose: Constructor.
ChatMessageModel::ChatMessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

// ##Method purpose: Returns the number of messages.
int ChatMessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) return 0;
    return m_messages.count();
}

// ##Method purpose: Returns data for the requested role at the given index.
QVariant ChatMessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.count())
        return QVariant();

    const Message &msg = m_messages.at(index.row());

    // ##Condition purpose: Map each custom role to the appropriate message field.
    switch (role) {
    case Qt::DisplayRole:
    case ContentRole:
        return msg.content;
    case RoleRole:
        return msg.role;
    case TimestampRole:
        return msg.timestamp;
    case IsStreamingRole:
        return msg.isStreaming;
    default:
        return QVariant();
    }
}

// ##Method purpose: Adds a complete message to the end of the model.
void ChatMessageModel::addMessage(const QString &role, const QString &content)
{
    int row = m_messages.count();
    beginInsertRows(QModelIndex(), row, row);
    Message msg;
    msg.role = role;
    msg.content = content;
    msg.timestamp = QDateTime::currentDateTime();
    msg.isStreaming = false;
    m_messages.append(msg);
    endInsertRows();
}

// ##Method purpose: Appends streaming tokens to the last assistant message, or creates one.
// If the last message is a "thinking" placeholder, it is replaced by the first real token.
void ChatMessageModel::appendToLastAssistant(const QString &token)
{
    // ##Condition purpose: Replace a "thinking" placeholder with the first real assistant content.
    if (!m_messages.isEmpty() && m_messages.last().role == QStringLiteral("thinking")) {
        m_messages.last().role = QStringLiteral("assistant");
        m_messages.last().content = token;
        m_messages.last().isStreaming = true;
        QModelIndex idx = index(m_messages.count() - 1);
        Q_EMIT dataChanged(idx, idx, {RoleRole, ContentRole, Qt::DisplayRole, IsStreamingRole});
        return;
    }

    // ##Condition purpose: Create a new streaming message if none exists or the last isn't an assistant.
    if (m_messages.isEmpty() || m_messages.last().role != QStringLiteral("assistant")) {
        int row = m_messages.count();
        beginInsertRows(QModelIndex(), row, row);
        Message msg;
        msg.role = QStringLiteral("assistant");
        msg.content = token;
        msg.timestamp = QDateTime::currentDateTime();
        msg.isStreaming = true;
        m_messages.append(msg);
        endInsertRows();
    } else {
        m_messages.last().content += token;
        QModelIndex idx = index(m_messages.count() - 1);
        Q_EMIT dataChanged(idx, idx, {ContentRole, Qt::DisplayRole});
    }
}

// ##Method purpose: Marks the last message as no longer streaming.
void ChatMessageModel::finaliseLastAssistant()
{
    if (!m_messages.isEmpty() && m_messages.last().isStreaming) {
        m_messages.last().isStreaming = false;
        QModelIndex idx = index(m_messages.count() - 1);
        Q_EMIT dataChanged(idx, idx, {IsStreamingRole});
    }
}

// ##Method purpose: Returns the last assistant message content for history serialisation.
QString ChatMessageModel::lastAssistantContent() const
{
    // ##Loop purpose: Walk backwards to find the latest assistant message.
    for (int i = m_messages.count() - 1; i >= 0; --i) {
        if (m_messages.at(i).role == QStringLiteral("assistant")) {
            return m_messages.at(i).content;
        }
    }
    return QString();
}

// ##Method purpose: Removes the trailing "thinking" placeholder if present.
void ChatMessageModel::removeThinkingIndicator()
{
    if (!m_messages.isEmpty() && m_messages.last().role == QStringLiteral("thinking")) {
        int row = m_messages.count() - 1;
        beginRemoveRows(QModelIndex(), row, row);
        m_messages.removeLast();
        endRemoveRows();
    }
}

// ##Method purpose: Removes all messages from the model.
void ChatMessageModel::clear()
{
    if (m_messages.isEmpty()) return;
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

// ##Method purpose: Read-only access to the full message list.
const QVector<ChatMessageModel::Message>& ChatMessageModel::messages() const
{
    return m_messages;
}

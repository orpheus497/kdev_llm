// ##Script function and purpose: Defines a native Qt delegate for rendering chat messages as styled bubbles in a QListView.
#pragma once

#include <QStyledItemDelegate>
#include <QCache>
#include <QTextDocument>

// ##Class purpose: Paints chat message bubbles using the native QPalette and QStyle, matching KDevelop's look and feel.
class ChatMessageDelegate : public QStyledItemDelegate {
    Q_OBJECT
public:
    // ##Method purpose: Constructor.
    explicit ChatMessageDelegate(QObject *parent = nullptr);

    // ##Method purpose: Paints a single chat message with role-appropriate styling.
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    // ##Method purpose: Calculates the required size for a message bubble.
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

private:
    // ##Method purpose: Computes the text layout width from the viewport width. Shared by paint() and sizeHint().
    int textLayoutWidth(int viewportWidth) const;

    // ##Method purpose: Creates a QTextDocument with the correct content, font, and wrapping width. Caller takes ownership.
    QTextDocument* createDoc(const QString &content, const QString &role,
                              const QFont &font, int layoutWidth) const;

    static constexpr int kBubblePadding = 10;
    static constexpr int kBubbleMargin = 6;
    static constexpr int kBubbleRadius = 8;
    static constexpr int kMaxBubbleWidthPercent = 85;
};

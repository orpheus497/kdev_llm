// ##Script function and purpose: Implements native chat bubble rendering using QPainter and the system palette.
#include "ChatMessageDelegate.h"
#include "ChatMessageModel.h"

#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>

// ##Method purpose: Constructor.
ChatMessageDelegate::ChatMessageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

// ##Method purpose: Paints a message bubble with role-specific colours drawn from the native palette.
void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);

    const QString role = index.data(ChatMessageModel::RoleRole).toString();
    const QString content = index.data(ChatMessageModel::ContentRole).toString();
    const bool isStreaming = index.data(ChatMessageModel::IsStreamingRole).toBool();

    const QPalette &pal = option.palette;
    const int availWidth = option.rect.width() - 2 * kBubbleMargin;
    const int maxBubbleWidth = availWidth * kMaxBubbleWidthPercent / 100;

    // ##Step purpose: Configure a QTextDocument for word-wrapped text rendering with markdown support.
    QTextDocument doc;
    doc.setDefaultFont(option.font);
    doc.setTextWidth(maxBubbleWidth - 2 * kBubblePadding);

    // ##Condition purpose: Use markdown rendering for assistant content, plain text for user messages.
    if (role == QStringLiteral("assistant")) {
        doc.setMarkdown(content, QTextDocument::MarkdownDialectGitHub);
    } else {
        doc.setPlainText(content);
    }

    const QSizeF docSize = doc.size();
    const int bubbleWidth = qMin(maxBubbleWidth, static_cast<int>(docSize.width()) + 2 * kBubblePadding);
    const int bubbleHeight = static_cast<int>(docSize.height()) + 2 * kBubblePadding;

    // ##Step purpose: Determine bubble alignment and colour based on the message role.
    QColor bubbleColor;
    int bubbleX;

    // ##Condition purpose: Style each role differently — user on the right, assistant on the left, system centred.
    if (role == QStringLiteral("user")) {
        bubbleColor = pal.color(QPalette::Active, QPalette::Highlight);
        bubbleX = option.rect.right() - kBubbleMargin - bubbleWidth;
    } else if (role == QStringLiteral("assistant")) {
        bubbleColor = pal.color(QPalette::Active, QPalette::AlternateBase);
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("error")) {
        // ##Step purpose: Errors use a desaturated red tint.
        QColor base = pal.color(QPalette::Active, QPalette::Window);
        bubbleColor = QColor::fromHsl(0, 80, qBound(40, base.lightness(), 200));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("warning")) {
        // ##Step purpose: Warnings use a desaturated amber tint.
        QColor base = pal.color(QPalette::Active, QPalette::Window);
        bubbleColor = QColor::fromHsl(40, 80, qBound(40, base.lightness(), 200));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("thinking")) {
        // ##Step purpose: Thinking indicator uses a subtle highlight tint to show active waiting.
        QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        bubbleColor = QColor::fromHsl(highlight.hslHue(), 40, qBound(60, pal.color(QPalette::Active, QPalette::Window).lightness() + 15, 220));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else {
        // system, welcome, etc. — subtle background
        bubbleColor = pal.color(QPalette::Active, QPalette::Window);
        bubbleX = option.rect.left() + kBubbleMargin;
    }

    const int bubbleY = option.rect.top() + kBubbleMargin;
    QRectF bubbleRect(bubbleX, bubbleY, bubbleWidth, bubbleHeight);

    // ##Step purpose: Draw the rounded bubble background.
    painter->setPen(Qt::NoPen);
    painter->setBrush(bubbleColor);
    painter->drawRoundedRect(bubbleRect, kBubbleRadius, kBubbleRadius);

    // ##Step purpose: Draw a subtle streaming indicator (pulsing dot) when the assistant is still generating.
    if (isStreaming) {
        painter->setBrush(pal.color(QPalette::Active, QPalette::Highlight));
        const int dotSize = 6;
        painter->drawEllipse(
            QPointF(bubbleRect.right() - kBubblePadding, bubbleRect.bottom() - kBubblePadding),
            dotSize / 2.0, dotSize / 2.0
        );
    }

    // ##Step purpose: Render the text content inside the bubble.
    // Choose text colour based on background luminance for readability.
    QColor textColor;
    if (role == QStringLiteral("user")) {
        textColor = pal.color(QPalette::Active, QPalette::HighlightedText);
    } else {
        textColor = pal.color(QPalette::Active, QPalette::Text);
    }

    painter->translate(bubbleRect.left() + kBubblePadding, bubbleRect.top() + kBubblePadding);

    // ##Step purpose: Apply the text colour to the document before painting.
    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, textColor);
    doc.documentLayout()->draw(painter, ctx);

    painter->restore();
}

// ##Method purpose: Computes the bounding rectangle for a message bubble.
QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString content = index.data(ChatMessageModel::ContentRole).toString();
    const QString role = index.data(ChatMessageModel::RoleRole).toString();

    const int availWidth = option.rect.width() > 0 ? option.rect.width() : 400;
    const int maxBubbleWidth = (availWidth - 2 * kBubbleMargin) * kMaxBubbleWidthPercent / 100;

    QTextDocument doc;
    doc.setDefaultFont(option.font);
    doc.setTextWidth(maxBubbleWidth - 2 * kBubblePadding);

    if (role == QStringLiteral("assistant")) {
        doc.setMarkdown(content, QTextDocument::MarkdownDialectGitHub);
    } else {
        doc.setPlainText(content);
    }

    const int height = static_cast<int>(doc.size().height()) + 2 * kBubblePadding + 2 * kBubbleMargin;
    return QSize(availWidth, height);
}

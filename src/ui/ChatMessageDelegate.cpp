// ##Script function and purpose: Implements native chat bubble rendering using QPainter and the system palette.
#include "ChatMessageDelegate.h"
#include "ChatMessageModel.h"

#include <QPainter>
#include <QTextDocument>
#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QListView>

// ##Method purpose: Constructor.
ChatMessageDelegate::ChatMessageDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

// ##Method purpose: Computes the text layout width from the overall available width.
// Extracted to a shared helper so paint() and sizeHint() always agree on dimensions.
int ChatMessageDelegate::textLayoutWidth(int viewportWidth) const
{
    const int maxBubbleWidth = (viewportWidth - 2 * kBubbleMargin) * kMaxBubbleWidthPercent / 100;
    return qMax(100, maxBubbleWidth - 2 * kBubblePadding);
}

// ##Method purpose: Creates and configures a QTextDocument with the proper content and wrapping width.
QTextDocument* ChatMessageDelegate::createDoc(const QString &content, const QString &role,
                                               const QFont &font, int layoutWidth) const
{
    auto *doc = new QTextDocument();
    doc->setDefaultFont(font);
    doc->setTextWidth(layoutWidth);
    doc->setDocumentMargin(0);

    // ##Step purpose: Ensure text wrapping properly handles long uninterrupted strings like URLs or code blocks.
    QTextOption opt = doc->defaultTextOption();
    opt.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    doc->setDefaultTextOption(opt);

    // ##Condition purpose: Use markdown for assistant content, plain text for everything else.
    if (role == QStringLiteral("assistant")) {
        doc->setMarkdown(content, QTextDocument::MarkdownDialectGitHub);
    } else {
        doc->setPlainText(content);
    }
    // ##Step purpose: Force layout so size() is accurate immediately.
    doc->documentLayout();
    doc->adjustSize();
    return doc;
}

// ##Method purpose: Paints a message bubble with role-specific colours drawn from the native palette.
void ChatMessageDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    // ##Step purpose: Clip to the item rectangle to prevent any overflow into adjacent items.
    painter->setClipRect(option.rect);

    const QString role = index.data(ChatMessageModel::RoleRole).toString();
    const QString content = index.data(ChatMessageModel::ContentRole).toString();
    const bool isStreaming = index.data(ChatMessageModel::IsStreamingRole).toBool();

    const QPalette &pal = option.palette;
    const int viewportWidth = option.rect.width();
    const int layoutW = textLayoutWidth(viewportWidth);

    // ##Step purpose: Build the text document with the shared helper.
    QTextDocument *doc = createDoc(content, role, option.font, layoutW);
    const QSizeF docSize = doc->size();
    const int maxBubbleWidth = (viewportWidth - 2 * kBubbleMargin) * kMaxBubbleWidthPercent / 100;
    const int bubbleWidth = qMin(maxBubbleWidth, static_cast<int>(docSize.width()) + 2 * kBubblePadding);
    const int bubbleHeight = static_cast<int>(docSize.height()) + 2 * kBubblePadding;

    // ##Step purpose: Determine bubble alignment and colour based on the message role.
    QColor bubbleColor;
    int bubbleX;

    // ##Condition purpose: Style each role differently — user on the right, assistant on the left.
    if (role == QStringLiteral("user")) {
        bubbleColor = pal.color(QPalette::Active, QPalette::Highlight);
        bubbleX = option.rect.right() - kBubbleMargin - bubbleWidth;
    } else if (role == QStringLiteral("assistant")) {
        bubbleColor = pal.color(QPalette::Active, QPalette::AlternateBase);
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("error")) {
        QColor base = pal.color(QPalette::Active, QPalette::Window);
        bubbleColor = QColor::fromHsl(0, 80, qBound(40, base.lightness(), 200));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("warning")) {
        QColor base = pal.color(QPalette::Active, QPalette::Window);
        bubbleColor = QColor::fromHsl(40, 80, qBound(40, base.lightness(), 200));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else if (role == QStringLiteral("thinking")) {
        QColor highlight = pal.color(QPalette::Active, QPalette::Highlight);
        bubbleColor = QColor::fromHsl(highlight.hslHue(), 40, qBound(60, pal.color(QPalette::Active, QPalette::Window).lightness() + 15, 220));
        bubbleX = option.rect.left() + kBubbleMargin;
    } else {
        bubbleColor = pal.color(QPalette::Active, QPalette::Window);
        bubbleX = option.rect.left() + kBubbleMargin;
    }

    const int bubbleY = option.rect.top() + kBubbleMargin;
    QRectF bubbleRect(bubbleX, bubbleY, bubbleWidth, bubbleHeight);

    // ##Step purpose: Draw the rounded bubble background.
    painter->setPen(Qt::NoPen);
    painter->setBrush(bubbleColor);
    painter->drawRoundedRect(bubbleRect, kBubbleRadius, kBubbleRadius);

    // ##Step purpose: Draw a streaming indicator dot when the assistant is still generating.
    if (isStreaming) {
        painter->setBrush(pal.color(QPalette::Active, QPalette::Highlight));
        const int dotSize = 6;
        painter->drawEllipse(
            QPointF(bubbleRect.right() - kBubblePadding, bubbleRect.bottom() - kBubblePadding),
            dotSize / 2.0, dotSize / 2.0
        );
    }

    // ##Step purpose: Render the text content inside the bubble.
    QColor textColor;
    if (role == QStringLiteral("user")) {
        textColor = pal.color(QPalette::Active, QPalette::HighlightedText);
    } else {
        textColor = pal.color(QPalette::Active, QPalette::Text);
    }

    painter->translate(bubbleRect.left() + kBubblePadding, bubbleRect.top() + kBubblePadding);
    // ##Step purpose: Clip text drawing to the bubble interior to prevent overflow.
    painter->setClipRect(QRectF(0, 0, bubbleWidth - 2 * kBubblePadding, bubbleHeight - 2 * kBubblePadding));

    QAbstractTextDocumentLayout::PaintContext ctx;
    ctx.palette.setColor(QPalette::Text, textColor);
    doc->documentLayout()->draw(painter, ctx);

    delete doc;
    painter->restore();
}

// ##Method purpose: Computes the bounding rectangle for a message bubble.
QSize ChatMessageDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    const QString content = index.data(ChatMessageModel::ContentRole).toString();
    const QString role = index.data(ChatMessageModel::RoleRole).toString();

    // ##Step purpose: Use the viewport width from the parent QListView for accurate first-render sizing.
    int viewportWidth = option.rect.width();
    if (viewportWidth <= 0) {
        auto *view = qobject_cast<const QListView *>(option.widget);
        if (view && view->viewport()) {
            viewportWidth = view->viewport()->width();
        }
    }
    // ##Condition purpose: Final fallback if viewport width is still unavailable.
    if (viewportWidth <= 0) {
        viewportWidth = 400;
    }

    const int layoutW = textLayoutWidth(viewportWidth);

    // ##Step purpose: Create the document with the exact same parameters as paint().
    QTextDocument *doc = createDoc(content, role, option.font, layoutW);
    const int height = static_cast<int>(doc->size().height()) + 2 * kBubblePadding + 2 * kBubbleMargin;
    delete doc;

    return QSize(viewportWidth, height);
}

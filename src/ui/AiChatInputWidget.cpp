// ##Script function and purpose: Implements the custom chat input area with @file completion.
#include "AiChatInputWidget.h"
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

#include <KLocalizedString>
// ============================================================================
// CommandTextEdit Implementation
// ============================================================================

// ##Method purpose: Constructor for the custom text edit.
CommandTextEdit::CommandTextEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

// ##Method purpose: Associates a QCompleter with this text edit.
void CommandTextEdit::setCompleter(QCompleter *completer)
{
    if (m_completer) {
        disconnect(m_completer, nullptr, this, nullptr);
    }
    m_completer = completer;
    if (!m_completer) return;
    
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);

    connect(m_completer, QOverload<const QString &>::of(&QCompleter::activated),
            this, &CommandTextEdit::insertCompletion);
}

// ##Method purpose: Sets the data model used for file autocompletion.
void CommandTextEdit::setModels(QAbstractItemModel *fileModel)
{
    m_fileModel = fileModel;
}

// ##Method purpose: Intercepts keystrokes to trigger the completer popup when typing '@'.
void CommandTextEdit::keyPressEvent(QKeyEvent *e)
{
    if (m_completer && m_completer->popup()->isVisible()) {
        switch (e->key()) {
        case Qt::Key_Enter:
        case Qt::Key_Return:
        case Qt::Key_Escape:
        case Qt::Key_Tab:
        case Qt::Key_Backtab:
            e->ignore();
            return;
        default:
            break;
        }
    }

    bool isShortcut = (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) && !(e->modifiers() & Qt::ShiftModifier);
    if (!m_completer || !isShortcut) {
        QTextEdit::keyPressEvent(e);
    }

    if (!m_completer) return;

    if (e->key() == Qt::Key_Backspace || e->key() == Qt::Key_Delete ||
        e->key() == Qt::Key_Left || e->key() == Qt::Key_Right ||
        e->key() == Qt::Key_Up || e->key() == Qt::Key_Down ||
        e->key() == Qt::Key_Home || e->key() == Qt::Key_End ||
        e->key() == Qt::Key_PageUp || e->key() == Qt::Key_PageDown) {
        return;
    }

    const bool ctrlOrShift = e->modifiers().testFlag(Qt::ControlModifier) || e->modifiers().testFlag(Qt::ShiftModifier);
    if (ctrlOrShift && e->text().isEmpty()) return;

    CompletionContext ctx = completionUnderCursor();

    if (ctx.type == File && m_fileModel) {
        m_completer->setModel(m_fileModel);
        m_completer->setFilterMode(Qt::MatchContains);
        m_completer->setCompletionPrefix(ctx.filterText);

        if (m_completer->completionCount() > 0 || ctx.filterText.isEmpty()) {
            QRect cr = cursorRect();
            cr.setWidth(m_completer->popup()->sizeHintForColumn(0) + m_completer->popup()->verticalScrollBar()->sizeHint().width());
            m_completer->complete(cr);
            m_completer->popup()->setCurrentIndex(m_completer->completionModel()->index(0, 0));
        } else {
            m_completer->popup()->hide();
        }
    } else {
        m_completer->popup()->hide();
    }
}

// ##Method purpose: Replaces the typed prefix with the selected completion text.
void CommandTextEdit::insertCompletion(const QString &completion)
{
    if (!m_completer) return;
    QTextCursor tc = textCursor();
    CompletionContext ctx = completionUnderCursor();
    if (ctx.type == None) return;

    tc.setPosition(ctx.prefixStart);
    tc.setPosition(textCursor().position(), QTextCursor::KeepAnchor);
    
    if (ctx.type == File) {
        tc.insertText(QStringLiteral("@") + completion);
    }
    setTextCursor(tc);
}

// ##Method purpose: Parses the text behind the cursor to detect if the user is typing a file reference.
CommandTextEdit::CompletionContext CommandTextEdit::completionUnderCursor() const
{
    CompletionContext context;
    context.type = None;
    context.prefixStart = -1;

    QTextCursor tc = textCursor();
    int cursorPos = tc.position();
    QString allText = toPlainText();

    int searchPos = cursorPos - 1;
    while (searchPos >= 0) {
        QChar ch = allText.at(searchPos);
        if (ch == QLatin1Char('@')) {
            if (searchPos == 0 || allText.at(searchPos - 1).isSpace()) {
                context.type = File;
                context.prefix = allText.mid(searchPos, cursorPos - searchPos);
                context.filterText = context.prefix.length() > 1 ? context.prefix.mid(1) : QString();
                context.prefixStart = searchPos;
                return context;
            } else {
                break;
            }
        }
        if (ch.isSpace()) break;
        searchPos--;
    }
    return context;
}

// ============================================================================
// AiChatInputWidget Implementation
// ============================================================================

// ##Method purpose: Initializes the layout and components for the chat input widget.
AiChatInputWidget::AiChatInputWidget(QWidget *parent)
    : QWidget(parent)
{
    auto *mainLayout = new QHBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    m_textEdit = new CommandTextEdit(this);
    m_textEdit->setPlaceholderText(i18n("Type a message... (@ for files)"));
    m_textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
    m_textEdit->installEventFilter(this);

    m_completer = new QCompleter(this);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchContains);
    m_completer->setWrapAround(false);
    m_textEdit->setCompleter(m_completer);

    auto *btnLayout = new QVBoxLayout();
    btnLayout->setContentsMargins(0, 0, 0, 0);
    
    m_sendButton = new QPushButton(QStringLiteral("Send"), this);
    m_sendButton->setToolTip(i18n("Send message (Enter)"));
    m_sendButton->setCursor(Qt::PointingHandCursor);
    m_newChatButton = new QPushButton(QStringLiteral("New Chat"), this);
    m_newChatButton->setToolTip(i18n("Clear history and start a new chat"));
    m_newChatButton->setCursor(Qt::PointingHandCursor);
    
    btnLayout->addWidget(m_sendButton);
    btnLayout->addWidget(m_newChatButton);
    
    mainLayout->addWidget(m_textEdit, 1);
    mainLayout->addLayout(btnLayout);

    connect(m_sendButton, &QPushButton::clicked, this, &AiChatInputWidget::onSendClicked);
    connect(m_newChatButton, &QPushButton::clicked, this, &AiChatInputWidget::onNewChatClicked);
}

// ##Method purpose: Enables or disables the input box and buttons.
void AiChatInputWidget::setPromptRunning(bool running)
{
    m_promptRunning = running;
    m_sendButton->setEnabled(!running);
    if (running) {
        m_sendButton->setToolTip(i18n("Waiting for AI response..."));
    } else {
        m_sendButton->setToolTip(i18n("Send message (Enter)"));
    }
}

// ##Method purpose: Clears the input box.
void AiChatInputWidget::clear()
{
    m_textEdit->clear();
}

// ##Method purpose: Returns the current input text.
QString AiChatInputWidget::text() const
{
    return m_textEdit->toPlainText();
}

// ##Method purpose: Provides the list of file paths to the autocompleter.
#include <QStringListModel>
void AiChatInputWidget::setAvailableFiles(const QStringList &files)
{
    if (m_fileModel) {
        delete m_fileModel;
    }
    m_fileModel = new QStringListModel(files, this);
    m_textEdit->setModels(m_fileModel);
}

// ##Method purpose: Captures Enter keys from the text area to trigger sending.
bool AiChatInputWidget::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_textEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if ((keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            if (!m_completer->popup()->isVisible() && !m_promptRunning) {
                onSendClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

// ##Method purpose: Emits the message and clears the input box.
void AiChatInputWidget::onSendClicked()
{
    QString message = m_textEdit->toPlainText().trimmed();
    if (!message.isEmpty()) {
        Q_EMIT messageSubmitted(message);
        m_textEdit->clear();
    }
}

// ##Method purpose: Triggers a new chat reset.
void AiChatInputWidget::onNewChatClicked()
{
    Q_EMIT newChatClicked();
}

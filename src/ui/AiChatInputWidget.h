// ##Script function and purpose: Provides a custom chat input area with @file completion.
#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QCompleter>

class QAbstractItemModel;
class QPushButton;

// ##Class purpose: Text editor that intercepts keystrokes to trigger QCompleter for @files.
class CommandTextEdit : public QTextEdit
{
    Q_OBJECT
public:
    explicit CommandTextEdit(QWidget *parent = nullptr);
    void setCompleter(QCompleter *completer);
    void setModels(QAbstractItemModel *fileModel);

protected:
    void keyPressEvent(QKeyEvent *e) override;

private Q_SLOTS:
    void insertCompletion(const QString &completion);

private:
    enum CompletionType { None, File };
    struct CompletionContext {
        CompletionType type;
        QString prefix;
        QString filterText;
        int prefixStart;
    };
    CompletionContext completionUnderCursor() const;
    QCompleter *m_completer = nullptr;
    QAbstractItemModel *m_fileModel = nullptr;
};

// ##Class purpose: Container widget that combines the input box and action buttons.
class AiChatInputWidget : public QWidget
{
    Q_OBJECT
public:
    explicit AiChatInputWidget(QWidget *parent = nullptr);
    void clear();
    QString text() const;
    void setAvailableFiles(const QStringList &files);
    void setPromptRunning(bool running);

Q_SIGNALS:
    void messageSubmitted(const QString &message);
    void newChatClicked();
    void stopClicked();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private Q_SLOTS:
    void onSendClicked();
    void onNewChatClicked();

private:
    CommandTextEdit *m_textEdit;
    QPushButton *m_sendButton;
    QPushButton *m_newChatButton;
    QCompleter *m_completer;
    QAbstractItemModel *m_fileModel = nullptr;
    bool m_promptRunning = false;
};

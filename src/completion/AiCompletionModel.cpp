// ##Script function and purpose: Implements the code completion data model logic.
#include "AiCompletionModel.h"
#include <KTextEditor/Document>
#include <KTextEditor/View>

// ##Method purpose: Constructor; initializes the LlamaClient and disables group headings.
AiCompletionModel::AiCompletionModel(QObject *parent) 
    : KTextEditor::CodeCompletionModel(parent)
    , m_client(new LlamaClient(this))
{
    setHasGroups(false);
    connect(m_client, &LlamaClient::completionReceived, this, &AiCompletionModel::onCompletionReceived);
    connect(m_client, &LlamaClient::errorOccurred, this, [this]() {
        m_isWaiting = false;
        beginResetModel();
        m_completions.clear();
        endResetModel();
    });
}

// ##Method purpose: Grabs the code context around the cursor and requests a completion from the LLM.
void AiCompletionModel::completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType)
{
    Q_UNUSED(invocationType);
    m_currentView = view;
    m_currentRange = range;
    
    beginResetModel();
    m_completions.clear();
    m_completions.append(QStringLiteral("Generating AI Suggestion..."));
    m_isWaiting = true;
    endResetModel();

    // ##Condition purpose: Abort if the view or document is invalid.
    if (!view || !view->document()) return;

    KTextEditor::Document *doc = view->document();
    QString prefix = doc->text(KTextEditor::Range(KTextEditor::Cursor(0, 0), range.end()));
    
    int maxLine = qMax(0, doc->lines() - 1);
    int endLine = qMin(maxLine, range.end().line() + 10);
    int endCol = doc->lineLength(endLine);
    QString suffix = doc->text(KTextEditor::Range(range.end(), KTextEditor::Cursor(endLine, endCol)));

    m_client->requestCompletion(prefix, suffix);
}

// ##Method purpose: Updates the internal model list when the LLM returns a full buffered suggestion.
void AiCompletionModel::onCompletionReceived(const QString &text)
{
    m_isWaiting = false;
    
    beginResetModel();
    m_completions.clear();
    
    if (!text.isEmpty() && m_currentView) {
        m_completions.append(text);
    }
    endResetModel();
}

// ##Method purpose: Required Qt model override; returns the list size.
int AiCompletionModel::rowCount(const QModelIndex &parent) const
{
    // ##Condition purpose: We do not support nested trees.
    if (parent.isValid()) return 0;
    return m_completions.count();
}

// ##Method purpose: Provides the completion string to the editor for the Name column.
QVariant AiCompletionModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_completions.count())
        return QVariant();

    if (role == Qt::DisplayRole || role == KTextEditor::CodeCompletionModel::CompletionRole) {
        if (index.column() == KTextEditor::CodeCompletionModel::Name) {
            QString fullText = m_completions.at(index.row());
            QString preview = fullText.split(QLatin1Char('\n')).first();
            if (fullText.contains(QLatin1Char('\n'))) {
                preview += QStringLiteral(" ...");
            }
            return preview;
        }
    }
    return QVariant();
}

// ##Method purpose: Injects the chosen completion text into the editor at the current cursor range.
void AiCompletionModel::executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const
{
    // ##Condition purpose: Ensure all indices and pointers are valid before modifying text.
    if (!index.isValid() || index.row() >= m_completions.count() || !view || !view->document() || m_isWaiting)
        return;

    QString text = m_completions.at(index.row());
    view->document()->replaceText(word, text);
}

// ##Script function and purpose: Defines the auto-completion provider for inline AI suggestions.
#pragma once
#include <KTextEditor/CodeCompletionModel>
#include <QPointer>
#include "../network/LlamaClient.h"

// ##Class purpose: Integrates with Kate's autocomplete popup to fetch and display AI code suggestions.
class AiCompletionModel : public KTextEditor::CodeCompletionModel {
    Q_OBJECT
    friend class TestAiCompletionModel;
public:
    // ##Method purpose: Constructor.
    explicit AiCompletionModel(QObject *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~AiCompletionModel() override = default;

    // ##Method purpose: Triggered by KTextEditor when autocomplete is invoked; starts the async LLM request.
    void completionInvoked(KTextEditor::View *view, const KTextEditor::Range &range, InvocationType invocationType) override;
    
    // ##Method purpose: Inserts the selected completion text into the document.
    void executeCompletionItem(KTextEditor::View *view, const KTextEditor::Range &word, const QModelIndex &index) const override;
    
    // ##Method purpose: Returns the display text for the autocomplete popup list.
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

private Q_SLOTS:
    // ##Method purpose: Callback triggered when the LlamaClient finishes an infill request.
    void onCompletionReceived(const QString &text);

private:
    LlamaClient *m_client;
    QPointer<KTextEditor::View> m_currentView;
    KTextEditor::Range m_currentRange;
    QStringList m_completions;
    bool m_isWaiting = false;
};

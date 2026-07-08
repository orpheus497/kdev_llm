// ##Script function and purpose: Manages the collection of context data (AGENTS.md, selection) for the LLM.
#pragma once
#include <QObject>
#include <QString>
#include <KTextEditor/Document>
#include <KTextEditor/View>

// ##Class purpose: Utility class for finding project roots and aggregating context for prompts.
class ContextManager : public QObject {
    Q_OBJECT
public:
    // ##Method purpose: Constructor.
    explicit ContextManager(QObject *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~ContextManager() override = default;

    // ##Method purpose: Finds the KDevelop project for a given URL, avoiding duplicate lookups.
    KDevelop::IProject* findProject(const QUrl &url) const;

    // ##Method purpose: Determines the project root directory from a given document.
    QString getProjectRoot(KTextEditor::Document *doc) const;

    // ##Method purpose: Determines the project root directory from a given URL (useful for testing).
    QString getProjectRoot(const QUrl &url) const;

    // ##Method purpose: Reads and returns the contents of AGENTS.md if found in the project root.
    QString getAgentsInstruction(const QString &projectRoot) const;

    // ##Method purpose: Assembles the final system prompt string containing the root instructions and selection.
    QString buildSystemPrompt(KTextEditor::View *view) const;

    // ##Method purpose: Builds a context-aware prompt for the AI refactoring action.
    QString buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const;
};

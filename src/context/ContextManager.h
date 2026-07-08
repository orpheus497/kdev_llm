// ##Script function and purpose: Manages the collection of context data (AGENTS.md, selection) for the LLM.
#pragma once
#include <QObject>
#include <QString>
#include <QHash>
#include <QDateTime>
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

    // ##Method purpose: Determines the project root directory from a given document.
    QString getProjectRoot(KTextEditor::Document *doc) const;

    // ##Method purpose: Reads and returns the contents of AGENTS.md if found in the project root.
    QString getAgentsInstruction(const QString &projectRoot) const;

    // ##Method purpose: Assembles the final system prompt string containing the root instructions and selection.
    QString buildSystemPrompt(KTextEditor::View *view) const;

    // ##Method purpose: Builds a context-aware prompt for the AI refactoring action.
    QString buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const;

private:
    struct AgentsCacheEntry {
        QDateTime lastModified;
        QString content;
    };

    // Caches to prevent redundant file system operations on every keystroke/message
    mutable QHash<QString, AgentsCacheEntry> m_agentsCache;
};

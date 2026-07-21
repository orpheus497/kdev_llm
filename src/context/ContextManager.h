// ##Script function and purpose: Manages the collection of context data (AGENTS.md, selection) for the LLM.
#pragma once
#include <QObject>
#include <QString>
#include <QUrl>
#include <QHash>
#include <QFileSystemWatcher>
#include <KTextEditor/Document>
#include <KTextEditor/View>

namespace KDevelop {
class IProject;
}

// ##Class purpose: Utility class for finding project roots and aggregating context for prompts.
class ContextManager : public QObject {
    Q_OBJECT
public:
    // ##Method purpose: Constructor.
    explicit ContextManager(QObject *parent = nullptr);
    
    // ##Method purpose: Destructor.
    ~ContextManager() override = default;

private:
    // ##Method purpose: Helper method to get the KDevelop project for a given URL.
    KDevelop::IProject* projectForUrl(const QUrl &url) const;

public:
    // ##Method purpose: Determines the project root directory from a given document.
    QString getProjectRoot(KTextEditor::Document *doc) const;

    // ##Method purpose: Reads and returns the contents of AGENTS.md if found in the project root.
    QString getAgentsInstruction(const QString &projectRoot) const;

    // ##Method purpose: Assembles the final system prompt string containing the root instructions and selection.
    QString buildSystemPrompt(KTextEditor::View *view) const;

    // ##Method purpose: Builds a context-aware prompt for the AI refactoring action.
    QString buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const;

    // ##Method purpose: Extracts relevant context (declarations, types) from a file using DUChain instead of dumping the full file.
    QString extractRelevantFileContext(const QString &filePath) const;

    // ##Method purpose: Returns the list of project file paths for @file autocompletion.
    QStringList getProjectFiles() const;

private Q_SLOTS:
    // ##Method purpose: Removes the specified file from the cache when it is modified.
    void onAgentsFileChanged(const QString &path);

private:
    mutable QHash<QString, QString> m_agentsCache;
    mutable QFileSystemWatcher* m_fileWatcher = nullptr;
};

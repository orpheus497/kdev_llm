// ##Script function and purpose: Implements the logic for scanning directories and formatting the system prompt.
#include "ContextManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/declaration.h>
#include <language/duchain/types/abstracttype.h>
#include <util/path.h>

// ##Method purpose: Constructor implementation.
ContextManager::ContextManager(QObject *parent) : QObject(parent) {}

QString ContextManager::getProjectRoot(KTextEditor::Document *doc) const
{
    if (!doc || doc->url().isEmpty()) {
        return QString();
    }
    
    // Attempt IDE proper integration first
    KDevelop::IProjectController* pc = KDevelop::ICore::self()->projectController();
    if (pc) {
        KDevelop::IProject* proj = pc->findProjectForUrl(doc->url());
        if (proj) {
            return proj->path().toLocalFile();
        }
    }
    
    // Fallback to directory scanning if not in a KDevelop project
    QDir dir = QFileInfo(doc->url().toLocalFile()).absoluteDir();
    while (dir.absolutePath() != QStringLiteral("/")) {
        if (dir.exists(QStringLiteral(".git")) || dir.exists(QStringLiteral("CMakeLists.txt"))) {
            return dir.absolutePath();
        }
        dir.cdUp();
    }
    return QString();
}

// ##Method purpose: Searches for AGENTS.md or .agents/AGENTS.md and reads it.
QString ContextManager::getAgentsInstruction(const QString &projectRoot) const
{
    // ##Condition purpose: Skip if project root is undefined.
    if (projectRoot.isEmpty()) return QString();

    QStringList candidates = {
        QStringLiteral("AGENTS.md"),
        QStringLiteral(".agents/AGENTS.md")
    };

    // ##Loop purpose: Check all possible locations for the AGENTS.md file.
    for (const auto &candidate : candidates) {
        QString filePath = QDir(projectRoot).filePath(candidate);
        QFileInfo fileInfo(filePath);
        
        if (fileInfo.exists()) {
            QDateTime lastModified = fileInfo.lastModified();

            // Check cache first
            auto it = m_agentsCache.constFind(filePath);
            if (it != m_agentsCache.constEnd() && it.value().lastModified == lastModified) {
                return it.value().content;
            }

            QFile file(filePath);
            // ##Condition purpose: Only read the file if we can successfully open it.
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();

                AgentsCacheEntry newEntry;
                newEntry.lastModified = lastModified;
                newEntry.content = content;
                m_agentsCache.insert(filePath, newEntry);

                return content;
            }
        }
    }
    return QString();
}

QString ContextManager::buildSystemPrompt(KTextEditor::View *view) const
{
    QString prompt = QStringLiteral("You are an expert AI coding assistant integrated natively into the KDevelop IDE.\n");
    
    if (view && view->document()) {
        QString root = getProjectRoot(view->document());
        QString agentsInst = getAgentsInstruction(root);
        
        KDevelop::IProjectController* pc = KDevelop::ICore::self()->projectController();
        if (pc) {
            KDevelop::IProject* proj = pc->findProjectForUrl(view->document()->url());
            if (proj) {
                prompt += QStringLiteral("Project Name: ") + proj->name() + QStringLiteral("\n");
                prompt += QStringLiteral("Project Root: ") + proj->path().toLocalFile() + QStringLiteral("\n\n");
            }
        }
        
        if (!agentsInst.isEmpty()) {
            prompt += QStringLiteral("Follow these project-specific instructions from AGENTS.md:\n");
            prompt += agentsInst + QStringLiteral("\n");
        }
        
        prompt += QStringLiteral("\nCurrent file: ") + view->document()->url().toLocalFile() + QStringLiteral("\n");
        prompt += QStringLiteral("\n--- File Content ---\n```\n");

        const int maxFileLength = 50000;
        KTextEditor::Document *doc = view->document();
        int totalLength = 0;
        int linesCount = doc->lines();
        int targetLine = 0;
        int targetColumn = 0;
        bool truncated = false;

        for (int i = 0; i < linesCount; ++i) {
            int len = doc->lineLength(i);
            if (totalLength + len + 1 > maxFileLength) {
                targetLine = i;
                targetColumn = maxFileLength - totalLength;
                truncated = true;
                break;
            }
            totalLength += len + 1;
        }

        if (truncated) {
            prompt += doc->text(KTextEditor::Range(0, 0, targetLine, targetColumn)) + QStringLiteral("\n...[Content truncated due to size]...\n");
        } else {
            prompt += doc->text();
        }

        prompt += QStringLiteral("\n```\n");
        
        if (view->selection()) {
            prompt += QStringLiteral("\nThe user has selected the following code:\n```\n");
            prompt += view->selectionText();
            prompt += QStringLiteral("\n```\n");
            
            // Try extracting semantic context from selection via DUChain
            KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
            KDevelop::DUChainUtils::ItemUnderCursor item = KDevelop::DUChainUtils::itemUnderCursor(view->document()->url(), view->selectionRange().start());
            if (item.declaration) {
                prompt += QStringLiteral("\nSemantic Information (from KDevelop DUChain AST):\n");
                prompt += QStringLiteral("- Declaration: ") + item.declaration->toString() + QStringLiteral("\n");
                if (item.declaration->abstractType()) {
                    prompt += QStringLiteral("- Type: ") + item.declaration->abstractType()->toString() + QStringLiteral("\n");
                }
            }
        }
    }
    
    return prompt;
}

QString ContextManager::buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const
{
    QString prompt = QStringLiteral("You are an expert developer. ");
    
    if (view && view->document()) {
        KDevelop::IProjectController* pc = KDevelop::ICore::self()->projectController();
        if (pc) {
            KDevelop::IProject* proj = pc->findProjectForUrl(view->document()->url());
            if (proj) {
                prompt += QStringLiteral("Project Name: ") + proj->name() + QStringLiteral("\n");
            }
        }
        
        prompt += QStringLiteral("You are working in the file: ") + view->document()->url().toLocalFile() + QStringLiteral("\n\n");
        prompt += QStringLiteral("Here is the full content of the file for context:\n```\n");

        const int maxFileLength = 50000;
        KTextEditor::Document *doc = view->document();
        int totalLength = 0;
        int linesCount = doc->lines();
        int targetLine = 0;
        int targetColumn = 0;
        bool truncated = false;

        for (int i = 0; i < linesCount; ++i) {
            int len = doc->lineLength(i);
            if (totalLength + len + 1 > maxFileLength) {
                targetLine = i;
                targetColumn = maxFileLength - totalLength;
                truncated = true;
                break;
            }
            totalLength += len + 1;
        }

        if (truncated) {
            prompt += doc->text(KTextEditor::Range(0, 0, targetLine, targetColumn)) + QStringLiteral("\n...[Content truncated due to size]...\n");
        } else {
            prompt += doc->text();
        }

        prompt += QStringLiteral("\n```\n\n");
        
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        KDevelop::DUChainUtils::ItemUnderCursor item = KDevelop::DUChainUtils::itemUnderCursor(view->document()->url(), view->selectionRange().start());
        if (item.declaration) {
            prompt += QStringLiteral("The selected code corresponds to the following semantic AST entity:\n");
            prompt += QStringLiteral("- Declaration: ") + item.declaration->toString() + QStringLiteral("\n");
            if (item.declaration->abstractType()) {
                prompt += QStringLiteral("- Type: ") + item.declaration->abstractType()->toString() + QStringLiteral("\n");
            }
            prompt += QStringLiteral("\n");
        }
    }
    
    prompt += QStringLiteral("The user has selected the following code to modify:\n```\n");
    prompt += code;
    prompt += QStringLiteral("\n```\n\n");
    
    prompt += QStringLiteral("Instruction: ") + instruction + QStringLiteral("\n\n");
    prompt += QStringLiteral("Please output ONLY the resulting modified code block to replace the selection. Do not include any conversational text or markdown wrappers in your output. Only raw code.");
    
    return prompt;
}

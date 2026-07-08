// ##Script function and purpose: Implements the logic for scanning directories and formatting the system prompt.
#include "ContextManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QDateTime>
#include <QReadWriteLock>
#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/declaration.h>
#include <language/duchain/types/abstracttype.h>
#include <util/path.h>
#include <QHash>
#include <QStringList>
#include <QDateTime>
#include <QRandomGenerator>
#include <QStringBuilder>

struct CacheEntry {
    QString rootPath;
    qint64 timestamp;
};

// Static cache with TTL to avoid redundant directory traversal and disk I/O
static QHash<QString, CacheEntry> s_projectRootCache;
const qint64 CACHE_TTL_MS = 5000; // 5 seconds TTL

// ##Method purpose: Constructor implementation.
ContextManager::ContextManager(QObject *parent) : QObject(parent) {}

KDevelop::IProject* ContextManager::findProject(const QUrl &url) const
{
    if (KDevelop::ICore::self()) {
        KDevelop::IProjectController* pc = KDevelop::ICore::self()->projectController();
        if (pc) {
            return pc->findProjectForUrl(url);
        }
    }
    return nullptr;
}

QString ContextManager::getProjectRoot(KTextEditor::Document *doc) const
{
    if (!doc || doc->url().isEmpty()) {
        return QString();
    }
    
    return getProjectRoot(doc->url());
}

QString ContextManager::getProjectRoot(const QUrl &url) const
{
    if (url.isEmpty()) {
        return QString();
    }

    // Attempt IDE proper integration first
    KDevelop::IProject* proj = findProject(url);
    if (proj) {
        return proj->path().toLocalFile();
    }
    
    // Fallback to directory scanning if not in a KDevelop project
    QDir dir = QFileInfo(doc->url().toLocalFile()).absoluteDir();
    QStringList visitedDirs;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // ##Step purpose: Clean up stale entries occasionally (simple opportunistic cleanup)
    if (QRandomGenerator::global()->bounded(100) == 0) {
        auto it = s_projectRootCache.begin();
        while (it != s_projectRootCache.end()) {
            if (now - it.value().timestamp >= CACHE_TTL_MS) {
                it = s_projectRootCache.erase(it);
            } else {
                ++it;
            }
        }
    }

    bool isRoot = false;
    // ##Loop purpose: Traverse up the directory tree until reaching the root or a project marker
    while (!isRoot) {
        QString currentPath = dir.absolutePath();

        // Check if we reached the filesystem root (platform-agnostic)
        isRoot = (dir.isRoot());

        // ##Condition purpose: Check if we have a valid cached root for this directory
        auto it = s_projectRootCache.constFind(currentPath);
        if (it != s_projectRootCache.constEnd()) {
            CacheEntry entry = it.value(); // Copy to avoid iterator invalidation during inserts
            if (now - entry.timestamp < CACHE_TTL_MS) {
                // Cache for all visited directories on the way up
                for (const QString& visited : visitedDirs) {
                    s_projectRootCache.insert(visited, {entry.rootPath, entry.timestamp});
                }
                return entry.rootPath;
            } else {
                // Evict the stale entry
                s_projectRootCache.remove(currentPath);
            }
        }

        visitedDirs.append(currentPath);

        // ##Condition purpose: Check if the current directory contains project markers (.git or CMakeLists.txt)
        if (dir.exists(QStringLiteral(".git")) || dir.exists(QStringLiteral("CMakeLists.txt"))) {
            QString rootPath = dir.absolutePath();
            // ##Loop purpose: Cache the resolved root for all directories visited on the way up
            for (const QString& visited : visitedDirs) {
                s_projectRootCache.insert(visited, {rootPath, now});
            }
            return rootPath;
        }

        if (!isRoot) {
            if (!dir.cdUp()) {
                break; // If we can't go up anymore despite not being root
            }
        }
    }

    // ##Loop purpose: Cache the empty result (no project root) for all visited directories to prevent redundant lookups
    for (const QString& visited : visitedDirs) {
        s_projectRootCache.insert(visited, {QString(), now});
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

    struct AgentCache {
        QReadWriteLock lock;
        QHash<QString, QString> content;
        QHash<QString, QDateTime> lastModified;
    };
    static AgentCache cache;

    // ##Loop purpose: Check all possible locations for the AGENTS.md file.
    for (const auto &candidate : candidates) {
        QString filePath = QDir(projectRoot).filePath(candidate);
        QFileInfo info(filePath);
        QDateTime lastMod = info.lastModified();
        
        if (lastMod.isValid()) {
            {
                QReadLocker locker(&cache.lock);
                if (cache.lastModified.value(filePath) == lastMod) {
                    return cache.content.value(filePath);
                }
            }

            QFile file(filePath);

            // ##Condition purpose: Only read the file if we can successfully open it.
            if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
                QTextStream in(&file);
                QString content = in.readAll();

                QWriteLocker locker(&cache.lock);
                cache.lastModified[filePath] = lastMod;
                cache.content[filePath] = content;
                return content;
            }
        }
    }
    return QString();
}

QString ContextManager::buildSystemPrompt(KTextEditor::View *view) const
{
    QString prompt;
    prompt.reserve(PromptReserveCapacity); // Pre-allocate to reduce memory reallocations
    prompt += QStringLiteral("You are an expert AI coding assistant integrated natively into the KDevelop IDE.\n");
    
    if (view && view->document()) {
        QString root = getProjectRoot(view->document());
        QString agentsInst = getAgentsInstruction(root);
        
        KDevelop::IProject* proj = findProject(view->document()->url());
        if (proj) {
            prompt += QStringLiteral("Project Name: ") % proj->name() % QChar('\n');
            prompt += QStringLiteral("Project Root: ") % proj->path().toLocalFile() % QStringLiteral("\n\n");
        }
        
        if (!agentsInst.isEmpty()) {
            prompt += QStringLiteral("Follow these project-specific instructions from AGENTS.md:\n");
            prompt += agentsInst % QChar('\n');
        }
        
        prompt += QStringLiteral("\nCurrent file: ") % view->document()->url().toLocalFile() % QChar('\n');
        prompt += QStringLiteral("\n--- File Content ---\n```\n");
        prompt += view->document()->text();
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
                prompt += QStringLiteral("- Declaration: ") % item.declaration->toString() % QChar('\n');
                if (item.declaration->abstractType()) {
                    prompt += QStringLiteral("- Type: ") % item.declaration->abstractType()->toString() % QChar('\n');
                }
            }
        }
    }
    
    return prompt;
}

QString ContextManager::buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const
{
    QString prompt;
    prompt.reserve(PromptReserveCapacity); // Pre-allocate to reduce memory reallocations
    prompt += QStringLiteral("You are an expert developer. ");
    
    if (view && view->document()) {
        KDevelop::IProject* proj = findProject(view->document()->url());
        if (proj) {
            prompt += QStringLiteral("Project Name: ") % proj->name() % QChar('\n');
        }
        
        prompt += QStringLiteral("You are working in the file: ") % view->document()->url().toLocalFile() % QStringLiteral("\n\n");
        prompt += QStringLiteral("Here is the full content of the file for context:\n```\n");
        prompt += view->document()->text();
        prompt += QStringLiteral("\n```\n\n");
        
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        KDevelop::DUChainUtils::ItemUnderCursor item = KDevelop::DUChainUtils::itemUnderCursor(view->document()->url(), view->selectionRange().start());
        if (item.declaration) {
            prompt += QStringLiteral("The selected code corresponds to the following semantic AST entity:\n");
            prompt += QStringLiteral("- Declaration: ") % item.declaration->toString() % QChar('\n');
            if (item.declaration->abstractType()) {
                prompt += QStringLiteral("- Type: ") % item.declaration->abstractType()->toString() % QChar('\n');
            }
            prompt += QStringLiteral("\n");
        }
    }
    
    prompt += QStringLiteral("The user has selected the following code to modify:\n```\n");
    prompt += code;
    prompt += QStringLiteral("\n```\n\n");
    
    prompt += QStringLiteral("Instruction: ") % instruction % QStringLiteral("\n\n");
    prompt += QStringLiteral("Please output ONLY the resulting modified code block to replace the selection. Do not include any conversational text or markdown wrappers in your output. Only raw code.");
    
    return prompt;
}

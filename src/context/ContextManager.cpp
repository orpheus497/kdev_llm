// ##Script function and purpose: Implements the logic for scanning directories and formatting the system prompt.
#include "ContextManager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/declaration.h>
#include <language/duchain/types/abstracttype.h>
#include <language/duchain/topducontext.h>
#include <project/projectmodel.h>
#include <util/path.h>
#include <QStringBuilder>
#include <QFileSystemWatcher>
#include <QSet>
#include <QStringView>
#include <utility>

// ##Method purpose: Helper to extract semantic context from KDevelop DUChain AST
static QString getSemanticASTString(KTextEditor::View *view, const QString &header)
{
    if (!view || !view->document()) return QString();

    KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
    KDevelop::DUChainUtils::ItemUnderCursor item = KDevelop::DUChainUtils::itemUnderCursor(view->document()->url(), view->selectionRange().start());

    if (item.declaration) {
        QString result = header % QStringLiteral("\n") %
                         QStringLiteral("- Declaration: ") % item.declaration->toString() % QStringLiteral("\n");
        if (auto type = item.declaration->abstractType()) {
            result += QStringLiteral("- Type: ") % type->toString() % QStringLiteral("\n");
        }
        return result;
    }
    return QString();
}

// ##Method purpose: Helper to truncate large documents to fit context limits
static QString getTruncatedDocumentText(KTextEditor::Document *doc, int maxLength)
{
    if (!doc) {
        return QString();
    }
    int totalLength = 0;
    int linesCount = doc->lines();
    int targetLine = 0;
    int targetColumn = 0;
    bool truncated = false;

    for (int i = 0; i < linesCount; ++i) {
        int len = doc->lineLength(i);
        if (totalLength + len + 1 > maxLength) {
            targetLine = i;
            targetColumn = maxLength - totalLength;
            truncated = true;
            break;
        }
        totalLength += len + 1;
    }

    if (truncated) {
        return doc->text(KTextEditor::Range(0, 0, targetLine, targetColumn)) % QStringLiteral("\n...[Content truncated due to size]...\n");
    }
    return doc->text();
}

// ##Method purpose: Constructor implementation.
ContextManager::ContextManager(QObject *parent) : QObject(parent) {}

KDevelop::IProject* ContextManager::projectForUrl(const QUrl &url) const
{
    if (url.isEmpty()) {
        return nullptr;
    }
    if (auto* core = KDevelop::ICore::self()) {
        if (auto* pc = core->projectController()) {
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
    
    // Attempt IDE proper integration first
    if (KDevelop::IProject* proj = projectForUrl(doc->url())) {
        return proj->path().toLocalFile();
    }
    
    // Fallback to directory scanning if not in a KDevelop project
    QDir dir = QFileInfo(doc->url().toLocalFile()).absoluteDir();
    while (true) {
        if (dir.exists(QStringLiteral(".git")) || dir.exists(QStringLiteral("CMakeLists.txt"))) {
            return dir.absolutePath();
        }
        if (dir.isRoot() || !dir.cdUp()) {
            break;
        }
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

    QDir rootDir(projectRoot);
    QString canonRoot = rootDir.canonicalPath();
    if (canonRoot.isEmpty()) return QString();

    // Ensure canonRoot ends with a separator so a sibling dir like project_secrets
    // doesn't pass the startsWith check for project. Use '/' explicitly as Qt
    // canonical paths always use forward slashes even on Windows.
    if (!canonRoot.endsWith(QLatin1Char('/'))) {
        canonRoot += QLatin1Char('/');
    }

    // ##Loop purpose: Check all possible locations for the AGENTS.md file.
    for (const auto &candidate : candidates) {
        QString filePath = rootDir.filePath(candidate);
        QFileInfo fi(filePath);

        QString canonFile = fi.canonicalFilePath();
        if (canonFile.isEmpty()) continue; // File does not exist

        // ##Condition purpose: Prevent path traversal via symlinks.
        if (!canonFile.startsWith(canonRoot)) continue;

        // ##Condition purpose: Return cached agent instructions if available.
        auto it = m_agentsCache.constFind(canonFile);
        if (it != m_agentsCache.constEnd()) {
            return *it;
        }

        QFile file(canonFile);
        
        // ##Condition purpose: Only read the file if we can successfully open it.
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&file);
            QString content = in.readAll();

            // ##Condition purpose: Ensure file is watched for changes to invalidate cache properly.
            if (!m_fileWatcher) {
                // ##Action purpose: Create the watcher instance.
                m_fileWatcher = new QFileSystemWatcher(const_cast<ContextManager*>(this));
                // ##Action purpose: Connect the file change signal to invalidate the cache.
                connect(m_fileWatcher, &QFileSystemWatcher::fileChanged,
                        this, &ContextManager::onAgentsFileChanged);
            }

            // ##Action purpose: Register the file path to be monitored.
            m_fileWatcher->addPath(canonFile);

            // ##Condition purpose: Cache the content only when the canonical file is successfully tracked by the watcher to guarantee its invalidation path.
            if (m_fileWatcher->files().contains(canonFile)) {
                // ##Action purpose: Cache the content now that an invalidation path is guaranteed.
                m_agentsCache.insert(canonFile, content);
            }

            return content;
        }
    }
    return QString();
}

// ##Method purpose: Slot to clear cache entry when the underlying AGENTS.md file changes on disk.
void ContextManager::onAgentsFileChanged(const QString &path)
{
    // ##Action purpose: Remove the cached entry for the modified file.
    m_agentsCache.remove(path);
}

QString ContextManager::buildSystemPrompt(KTextEditor::View *view) const
{
    QString prompt = QStringLiteral("You are an expert AI coding assistant integrated natively into the KDevelop IDE.\n");
    
    if (view && view->document()) {
        QString root = getProjectRoot(view->document());
        QString agentsInst = getAgentsInstruction(root);
        
        if (KDevelop::IProject* proj = projectForUrl(view->document()->url())) {
            prompt += QStringLiteral("Project Name: ") % proj->name() % QStringLiteral("\n") %
                      QStringLiteral("Project Root: ") % proj->path().toLocalFile() % QStringLiteral("\n\n");
        }
        
        if (!agentsInst.isEmpty()) {
            prompt += QStringLiteral("Follow these project-specific instructions from AGENTS.md:\n") %
                      agentsInst % QStringLiteral("\n");
        }
        
        prompt += QStringLiteral("\nCurrent file: ") % view->document()->url().toLocalFile() % QStringLiteral("\n") %
                  QStringLiteral("\n--- File Content ---\n```\n");

        const int maxFileLength = 50000;
        prompt += getTruncatedDocumentText(view->document(), maxFileLength);

        prompt += QStringLiteral("\n```\n");
        
        if (view->selection()) {
            prompt += QStringLiteral("\nThe user has selected the following code:\n```\n") %
                      view->selectionText() %
                      QStringLiteral("\n```\n");
            
            // Try extracting semantic context from selection via DUChain
            prompt += getSemanticASTString(view, QStringLiteral("\nSemantic Information (from KDevelop DUChain AST):"));
        }
    }
    
    return prompt;
}

QString ContextManager::buildRefactorPrompt(const QString &instruction, const QString &code, KTextEditor::View *view) const
{
    QString prompt = QStringLiteral("You are an expert developer. ");
    
    if (view && view->document()) {
        if (KDevelop::IProject* proj = projectForUrl(view->document()->url())) {
            prompt += QStringLiteral("Project Name: ") % proj->name() % QStringLiteral("\n");
        }
        
        prompt += QStringLiteral("You are working in the file: ") % view->document()->url().toLocalFile() % QStringLiteral("\n\n") %
                  QStringLiteral("Here is the full content of the file for context:\n```\n");

        const int maxFileLength = 50000;
        prompt += getTruncatedDocumentText(view->document(), maxFileLength);

        prompt += QStringLiteral("\n```\n\n");
        
        QString astInfo = getSemanticASTString(view, QStringLiteral("The selected code corresponds to the following semantic AST entity:"));
        if (!astInfo.isEmpty()) {
            prompt += astInfo % QStringLiteral("\n");
        }
    }
    
    prompt += QStringLiteral("The user has selected the following code to modify:\n```\n") %
              code %
              QStringLiteral("\n```\n\n") %
              QStringLiteral("Instruction: ") % instruction % QStringLiteral("\n\n") %
              QStringLiteral("Please output ONLY the resulting modified code block to replace the selection. Do not include any conversational text or markdown wrappers in your output. Only raw code.");
    
    return prompt;
}

// ##Method purpose: Extracts relevant declarations and structure from a file using DUChain.
QString ContextManager::extractRelevantFileContext(const QString &filePath) const
{
    // ##Condition purpose: Return empty if path is invalid.
    if (filePath.isEmpty()) return QString();

    QUrl fileUrl = QUrl::fromLocalFile(filePath);

    // ##Step purpose: Try DUChain first for semantic context.
    {
        KDevelop::DUChainReadLocker lock(KDevelop::DUChain::lock());
        auto *topContext = KDevelop::DUChain::self()->chainForDocument(fileUrl);
        if (topContext) {
            QString result = QStringLiteral("File: ") % filePath % QStringLiteral("\nDeclarations:\n");
            const auto decls = topContext->localDeclarations();
            // ##Loop purpose: Iterate top-level declarations in the file.
            for (auto *decl : decls) {
                result += QStringLiteral("  - ") % decl->toString();
                if (auto type = decl->abstractType()) {
                    result += QStringLiteral(" : ") % type->toString();
                }
                result += QStringLiteral("\n");
            }
            return result;
        }
    }

    // ##Step purpose: Fallback — read first 200 lines of the file if DUChain has no data.
    QFile file(filePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        QString content;
        int lineCount = 0;
        // ##Loop purpose: Read up to 200 lines as a fallback.
        while (!in.atEnd() && lineCount < 200) {
            content += in.readLine() % QStringLiteral("\n");
            ++lineCount;
        }
        if (!in.atEnd()) {
            content += QStringLiteral("\n...[Truncated]...\n");
        }
        return QStringLiteral("File: ") % filePath % QStringLiteral("\n```\n") % content % QStringLiteral("```\n");
    }
    return QString();
}

// ##Method purpose: Returns the list of project source file paths for @file autocompletion.
QStringList ContextManager::getProjectFiles() const
{
    QStringList files;
    auto *core = KDevelop::ICore::self();
    // ##Condition purpose: Only proceed if KDevelop core is available.
    if (!core || !core->projectController()) return files;

    auto *project = core->projectController()->findProjectForUrl(QUrl());
    // ##Condition purpose: Try the first open project if no URL match.
    if (!project) {
        auto projects = core->projectController()->projects();
        if (!projects.isEmpty()) {
            project = projects.first();
        }
    }
    if (!project) return files;

    // ##Step purpose: Recursively collect all file items from the project model.
    const auto allFiles = project->fileSet();

    // ##Step purpose: ⚡ Bolt: Use a QSet of QStringView for O(1) extension lookups instead of chained O(N) string comparisons.
    // This provides a measurable speedup during large project traversals by avoiding repeated `endsWith` calls.
    static const QSet<QStringView> allowedExtensions = {
        u"cpp", u"h", u"c", u"hpp", u"py", u"js", u"ts", u"java",
        u"rs", u"go", u"cmake", u"txt", u"md", u"json", u"xml", u"yaml", u"yml"
    };

    // ##Loop purpose: Iterate through project files to collect source-like code files.
    for (const auto &indexedString : allFiles) {
        QString path = indexedString.str();

        // ##Condition purpose: Guard against files with no extension to prevent out-of-bounds slicing.
        const int dotIndex = path.lastIndexOf(QLatin1Char('.'));
        if (dotIndex != -1) {
            // ##Step purpose: Extract the file extension string view efficiently without allocating new memory.
            const QStringView suffix = QStringView(path).mid(dotIndex + 1);
            // ##Condition purpose: Only include source-like files, not build artifacts.
            if (allowedExtensions.contains(suffix)) {
                // ##Step purpose: Move the string into the file list to prevent a redundant atomic reference count bump.
                files.append(std::move(path));
            }
        }
    }
    return files;
}

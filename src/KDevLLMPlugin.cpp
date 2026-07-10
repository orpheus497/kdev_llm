// ##Class purpose: Main entry point implementation for the KDev LLM plugin.
#include "KDevLLMPlugin.h"

#include <KPluginFactory>
#include <QDebug>
#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>
#include <interfaces/contextmenuextension.h>
#include <language/interfaces/editorcontext.h>
#include <KTextEditor/Editor>
#include <KTextEditor/View>
#include <KTextEditor/Document>
#include <KTextEditor/Message>
#include <QInputDialog>
#include <QAction>
#include <KLocalizedString>

#include "ui/AiChatWidget.h"
#include "completion/AiCompletionModel.h"
#include "network/LlamaClient.h"
#include "context/ContextManager.h"

class AiToolViewFactory : public KDevelop::IToolViewFactory {
public:
    AiToolViewFactory(KDevLLMPlugin* plugin) : m_plugin(plugin) {}
    QWidget* create(QWidget *parent = nullptr) override {
        auto* widget = new AiChatWidget(parent);
        widget->setWindowTitle(QStringLiteral("KDev LLM"));
        return widget;
    }
    QString id() const override { return QStringLiteral("org.kdevelop.KDevLLMPlugin"); }
    Qt::DockWidgetArea defaultPosition() const override { return Qt::RightDockWidgetArea; }
private:
    KDevLLMPlugin* m_plugin;
};

K_PLUGIN_FACTORY_WITH_JSON(KDevLLMPluginFactory, "kdevllm.json", registerPlugin<KDevLLMPlugin>();)

KDevLLMPlugin::KDevLLMPlugin(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args)
    : KDevelop::IPlugin(QStringLiteral("kdevllm"), parent, metaData)
    , m_completionModel(new AiCompletionModel(this))
    , m_refactorClient(new LlamaClient(this))
{
    Q_UNUSED(args);
    qDebug() << "KDev LLM Plugin Loaded!";
    m_factory = new AiToolViewFactory(this);
    core()->uiController()->addToolView(QStringLiteral("KDev LLM"), m_factory);
    // Force the tool view to be created and raised in the UI

    connect(m_refactorClient, &LlamaClient::refactorReceived, this, &KDevLLMPlugin::onRefactorReceived);
    connect(m_refactorClient, &LlamaClient::errorOccurred, this, [this](const QString &err) {
        if (m_refactorDocument && m_currentRefactorRange) {
            delete m_currentRefactorRange;
            m_currentRefactorRange = nullptr;
            auto *msg = new KTextEditor::Message(i18n("AI Refactor failed: %1", err), KTextEditor::Message::Error);
            msg->setPosition(KTextEditor::Message::TopInView);
            m_refactorDocument->postMessage(msg);
        }
    });

    // Register completion model via KDevelop DocumentController
    auto setupTextDocument = [this](KDevelop::IDocument* doc) {
        if (auto* textDoc = doc->textDocument()) {
            connect(textDoc, &KTextEditor::Document::viewCreated, this, [this](KTextEditor::Document*, KTextEditor::View* view) {
                view->registerCompletionModel(m_completionModel);
            });
            for (auto* view : textDoc->views()) {
                view->registerCompletionModel(m_completionModel);
            }
        }
    };

    connect(core()->documentController(), &KDevelop::IDocumentController::textDocumentCreated, this, setupTextDocument);
    
    for (auto* doc : core()->documentController()->openDocuments()) {
        setupTextDocument(doc);
    }
}

KDevLLMPlugin::~KDevLLMPlugin()
{
}

void KDevLLMPlugin::unload()
{
    qDebug() << "KDev LLM Plugin Unloaded!";
    core()->uiController()->removeToolView(m_factory);
    delete m_factory;
    
    for (auto* doc : KTextEditor::Editor::instance()->documents()) {
        for (auto* view : doc->views()) {
            view->unregisterCompletionModel(m_completionModel);
        }
    }
}

KDevelop::ContextMenuExtension KDevLLMPlugin::contextMenuExtension(KDevelop::Context* context, QWidget* parent)
{
    KDevelop::ContextMenuExtension ext;
    if (context->type() == KDevelop::Context::EditorContext) {
        auto* editorContext = static_cast<KDevelop::EditorContext*>(context);
        KTextEditor::View* view = editorContext->view();
        
        QAction* refactorAction = new QAction(i18n("AI: Refactor Selection..."), parent);
        connect(refactorAction, &QAction::triggered, this, [this, view]() {
            requestAiRefactor(view);
        });
        ext.addAction(KDevelop::ContextMenuExtension::RefactorGroup, refactorAction);
    }
    return ext;
}

void KDevLLMPlugin::requestAiRefactor(KTextEditor::View* view)
{
    if (!view || !view->document() || !view->selection()) {
        return;
    }
    
    bool ok;
    QString instruction = QInputDialog::getText(
        view,
        i18n("AI Refactor"),
        i18n("What should the AI do with this selection? (e.g. 'Optimize', 'Rename X to Y')"),
        QLineEdit::Normal,
        QString(),
        &ok
    );
    
    if (!ok || instruction.isEmpty()) {
        return;
    }
    
    if (m_refactorDocument && m_currentRefactorRange) {
        delete m_currentRefactorRange;
    }
    
    m_refactorDocument = view->document();
    m_currentRefactorRange = m_refactorDocument->newMovingRange(view->selectionRange());
    
    ContextManager contextMgr(this);
    QString promptText = contextMgr.buildRefactorPrompt(instruction, view->selectionText(), view);
    
    m_refactorClient->requestRefactor(promptText);
}

void KDevLLMPlugin::onRefactorReceived(const QString &text)
{
    if (!m_refactorDocument) {
        m_currentRefactorRange = nullptr;
        return;
    }
    
    if (m_currentRefactorRange) {
        m_refactorDocument->replaceText(m_currentRefactorRange->toRange(), text);
        delete m_currentRefactorRange;
        m_currentRefactorRange = nullptr;
    }
    
    auto *msg = new KTextEditor::Message(i18n("AI Refactor applied. Press Ctrl+Z to undo."), KTextEditor::Message::Information);
    msg->setPosition(KTextEditor::Message::TopInView);
    m_refactorDocument->postMessage(msg);
}

#include "KDevLLMPlugin.moc"

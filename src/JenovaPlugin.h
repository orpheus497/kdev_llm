// ##Class purpose: Main entry point for the Jenova KDevelop plugin.
#pragma once

#include <interfaces/iplugin.h>
#include <QVariantList>

#include <QPointer>

class KPluginMetaData;
class AiToolViewFactory;
class AiCompletionModel;
class LlamaClient;

namespace KTextEditor { 
    class View; 
    class Document; 
    class MovingRange; 
}

class JenovaPlugin : public KDevelop::IPlugin {
    Q_OBJECT

public:
    JenovaPlugin(QObject* parent, const KPluginMetaData& metaData, const QVariantList& args);
    ~JenovaPlugin() override;

    void unload() override;
    KDevelop::ContextMenuExtension contextMenuExtension(KDevelop::Context* context, QWidget* parent) override;

private Q_SLOTS:
    void requestAiRefactor(KTextEditor::View* view);
    void onRefactorReceived(const QString &text);

private:
    AiToolViewFactory* m_factory;
    AiCompletionModel* m_completionModel;
    LlamaClient* m_refactorClient;
    KTextEditor::MovingRange* m_currentRefactorRange = nullptr;
    QPointer<KTextEditor::Document> m_refactorDocument;
};

// ##Script function and purpose: Defines the KDevelop settings page for KDev LLM plugin configuration.
#pragma once

#include <interfaces/configpage.h>
#include <QPointer>

class QLineEdit;
class QSpinBox;
class LlamaClient;

// ##Class purpose: Provides a native KDevelop settings page for configuring the LLM endpoint, model, and timeout.
class AiConfigPage : public KDevelop::ConfigPage {
    Q_OBJECT
public:
    // ##Method purpose: Constructor.
    explicit AiConfigPage(KDevelop::IPlugin *plugin, QWidget *parent = nullptr);
    // ##Method purpose: Destructor.
    ~AiConfigPage() override = default;

    // ##Method purpose: Returns the display name for this config page.
    QString name() const override;
    // ##Method purpose: Returns the icon for this config page.
    QIcon icon() const override;
    // ##Method purpose: Returns the full display name including category.
    QString fullName() const override;

public Q_SLOTS:
    // ##Method purpose: Applies the settings to persistent storage and live LlamaClient instances.
    void apply() override;
    // ##Method purpose: Resets the UI to default values.
    void defaults() override;
    // ##Method purpose: Reloads settings from persistent storage.
    void reset() override;

private:
    // ##Method purpose: Internal helper to populate form fields from KSharedConfig without triggering base class reset().
    void loadSettings();

    QLineEdit *m_endpointUrl;
    QSpinBox *m_timeout;
    QLineEdit *m_modelName;
};

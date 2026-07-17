// ##Script function and purpose: Implements the KDevelop settings page for KDev LLM plugin configuration.
#include "AiConfigPage.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QSpinBox>
#include <QLabel>
#include <QIcon>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KLocalizedString>

// ##Method purpose: Constructs the settings form with endpoint, model, and timeout fields.
AiConfigPage::AiConfigPage(KDevelop::IPlugin *plugin, QWidget *parent)
    : KDevelop::ConfigPage(plugin, nullptr, parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *form = new QFormLayout();

    m_endpointUrl = new QLineEdit(this);
    m_endpointUrl->setPlaceholderText(QStringLiteral("http://127.0.0.1:8080"));
    m_endpointUrl->setToolTip(i18n("The base URL of your llama.cpp or compatible LLM server"));
    form->addRow(i18n("Endpoint URL:"), m_endpointUrl);

    m_modelName = new QLineEdit(this);
    m_modelName->setPlaceholderText(QStringLiteral("(server default)"));
    m_modelName->setToolTip(i18n("Optional model name to request from the server"));
    form->addRow(i18n("Model:"), m_modelName);

    m_timeout = new QSpinBox(this);
    m_timeout->setRange(10, 3600);
    m_timeout->setValue(600);
    m_timeout->setSuffix(QStringLiteral(" s"));
    m_timeout->setToolTip(i18n("Maximum time to wait for a response from the LLM server"));
    form->addRow(i18n("Request Timeout:"), m_timeout);

    layout->addLayout(form);

    // ##Step purpose: Add a help label at the bottom.
    auto *helpLabel = new QLabel(i18n("Changes take effect on the next request. "
                                      "Restart KDevelop if the endpoint URL was changed."), this);
    helpLabel->setWordWrap(true);
    helpLabel->setEnabled(false);
    layout->addWidget(helpLabel);

    layout->addStretch();

    // ##Step purpose: Populate form fields from saved config. 
    // NOTE: Do NOT call reset() here — KDevelop's ConfigDialog calls it
    // after construction via initConfigManager(). Calling it here with a
    // null KCoreConfigSkeleton would invoke the base-class reset() which
    // dereferences a null KConfigDialogManager and crashes.
    loadSettings();
}

// ##Method purpose: Returns the display name.
QString AiConfigPage::name() const
{
    return i18n("KDev LLM");
}

// ##Method purpose: Returns the icon.
QIcon AiConfigPage::icon() const
{
    return QIcon::fromTheme(QStringLiteral("code-context"));
}

// ##Method purpose: Returns the full display name.
QString AiConfigPage::fullName() const
{
    return i18n("KDev LLM — AI Assistant Settings");
}

// ##Method purpose: Persists the current form values to KSharedConfig.
void AiConfigPage::apply()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group = config->group(QStringLiteral("KDevLLM"));
    group.writeEntry("EndpointUrl", m_endpointUrl->text().trimmed());
    group.writeEntry("ModelName", m_modelName->text().trimmed());
    group.writeEntry("Timeout", m_timeout->value());
    config->sync();
}

// ##Method purpose: Resets the form to default values.
void AiConfigPage::defaults()
{
    m_endpointUrl->setText(QStringLiteral("http://127.0.0.1:8080"));
    m_modelName->clear();
    m_timeout->setValue(600);
}

// ##Method purpose: Reloads form values from persistent storage (called by KDevelop's ConfigDialog).
void AiConfigPage::reset()
{
    loadSettings();
}

// ##Method purpose: Internal helper to populate form fields from KSharedConfig.
void AiConfigPage::loadSettings()
{
    KSharedConfig::Ptr config = KSharedConfig::openConfig();
    KConfigGroup group = config->group(QStringLiteral("KDevLLM"));
    m_endpointUrl->setText(group.readEntry("EndpointUrl", QStringLiteral("http://127.0.0.1:8080")));
    m_modelName->setText(group.readEntry("ModelName", QString()));
    m_timeout->setValue(group.readEntry("Timeout", 600));
}

#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "sql.h"

#include <QSystemTrayIcon>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    proxyPreference = new QButtonGroup(this);
    proxyPreference->addButton(ui->m_proxyPreference_no);
    proxyPreference->addButton(ui->m_proxyPreference_system);
    proxyPreference->addButton(ui->m_proxyPreference_manual);

    QStringList proxyTypes;
    proxyTypes << "HTTP";
    proxyTypes << "SOCKS5";

    QIntValidator *portValdidator = new QIntValidator(1, 9999, this);

    ui->m_port->setValidator(portValdidator);
    ui->m_httpsPort->setValidator(portValdidator);

    ui->enableSytemTray->setEnabled(QSystemTrayIcon::isSystemTrayAvailable());
    ui->enableSytemTray->setChecked(sql::readSyncStatus("systemTray", true).toBool());

    ui->SyncInterval->setValue(sql::readSyncStatus("SyncInterval", 10).toInt());

    QString proxyPreference = sql::readSyncStatus("proxyPreference", "system").toString();
    if (proxyPreference == "system")
        ui->m_proxyPreference_system->setChecked(true);
    else if (proxyPreference == "manual")
        ui->m_proxyPreference_manual->setChecked(true);
    else
        ui->m_proxyPreference_no->setChecked(true);

    ui->m_proxyType->setCurrentIndex(proxyTypes.indexOf(sql::readSyncStatus("proxyType", "HTTP").toString()));
    ui->m_hostName->setText(sql::readSyncStatus("proxyHostName", "").toString());
    ui->m_port->setText(sql::readSyncStatus("proxyPort", "80").toString());
    ui->m_username->setText(sql::readSyncStatus("proxyUserName", "").toString());
    ui->m_password->setText(sql::readSyncStatus("proxyPassword", "").toString());
    ui->m_useDifferentProxyForHttps->setChecked(sql::readSyncStatus("useDifferentProxyForHttps", false).toBool());
    ui->m_httpsHostName->setText(sql::readSyncStatus("proxyHttpsHostName", "").toString());
    ui->m_httpsPort->setText(sql::readSyncStatus("proxyHttpsPort", "443").toString());
    ui->m_httpsUsername->setText(sql::readSyncStatus("proxyHttpsUsername", "").toString());
    ui->m_httpsPassword->setText(sql::readSyncStatus("proxyHttpsPassword", "").toString());

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(acceptConfig()));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::acceptConfig() {

    sql::updateSyncStatus("systemTray", ui->enableSytemTray->isChecked());
    sql::updateSyncStatus("SyncInterval", ui->SyncInterval->value());
    sql::updateSyncStatus("proxyPreference", proxyPreference->checkedButton()->objectName().split('_')[2]);
    sql::updateSyncStatus("proxyType", ui->m_proxyType->currentText());
    sql::updateSyncStatus("proxyHostName", ui->m_hostName->text());
    sql::updateSyncStatus("proxyPort", ui->m_port->text());
    sql::updateSyncStatus("proxyUserName", ui->m_username->text());
    sql::updateSyncStatus("proxyPassword", ui->m_password->text());
    sql::updateSyncStatus("useDifferentProxyForHttps", ui->m_useDifferentProxyForHttps->isChecked());
    sql::updateSyncStatus("proxyHttpsHostName", ui->m_httpsHostName->text());
    sql::updateSyncStatus("proxyHttpsPort", ui->m_httpsPort->text());
    sql::updateSyncStatus("proxyHttpsUsername", ui->m_httpsUsername->text());
    sql::updateSyncStatus("proxyHttpsPassword", ui->m_httpsPassword->text());

    settings["systemTray"] = ui->enableSytemTray->isChecked();
    settings["SyncInterval"] = ui->SyncInterval->value();

    accept();
}

QVariantMap OptionsDialog::getSettings() {
    return settings;
}

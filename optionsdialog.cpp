#include "optionsdialog.h"
#include "ui_optionsdialog.h"
#include "sql.h"

#include <QSystemTrayIcon>

OptionsDialog::OptionsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OptionsDialog)
{
    ui->setupUi(this);

    ui->enableSytemTray->setEnabled(QSystemTrayIcon::isSystemTrayAvailable());
    ui->enableSytemTray->setChecked(sql::readSyncStatus("systemTray", true).toBool());

    ui->SyncInterval->setValue(sql::readSyncStatus("SyncInterval", 10).toInt());

    connect(ui->buttonBox, SIGNAL(accepted()), this, SLOT(acceptConfig()));
}

OptionsDialog::~OptionsDialog()
{
    delete ui;
}

void OptionsDialog::acceptConfig() {

    sql::updateSyncStatus("systemTray", ui->enableSytemTray->isChecked());
    sql::updateSyncStatus("SyncInterval", ui->SyncInterval->value());

    settings["systemTray"] = ui->enableSytemTray->isChecked();
    settings["SyncInterval"] = ui->SyncInterval->value();

    accept();
}

QVariantMap OptionsDialog::getSettings() {
    return settings;
}

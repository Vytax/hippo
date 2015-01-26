#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QVariant>
#include <QButtonGroup>

namespace Ui {
class OptionsDialog;
}

class OptionsDialog : public QDialog
{
    Q_OBJECT

public:
    explicit OptionsDialog(QWidget *parent = 0);
    ~OptionsDialog();

    QVariantMap getSettings();
    void selectTabByName(QString name);

private:
    Ui::OptionsDialog *ui;
    QVariantMap settings;

    QButtonGroup *proxyPreference;

private slots:
    void acceptConfig();
};

#endif // OPTIONSDIALOG_H

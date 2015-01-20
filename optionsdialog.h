#ifndef OPTIONSDIALOG_H
#define OPTIONSDIALOG_H

#include <QDialog>
#include <QVariant>

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

private:
    Ui::OptionsDialog *ui;
    QVariantMap settings;

private slots:
    void acceptConfig();
};

#endif // OPTIONSDIALOG_H

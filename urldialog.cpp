#include "urldialog.h"

#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QIcon>
#include <QtSingleApplication>
#include <QClipboard>
#include <QUrl>

#include <QDebug>

URLDialog::URLDialog(QWidget *parent) :
    QDialog(parent)
{
    alowedSchemas << "http" << "https";

    QString clip;
    QUrl u(qApp->clipboard()->text());

    if (u.isValid() && alowedSchemas.contains(u.scheme()))
        clip = u.toString();

    setWindowIcon(QIcon(":/img/evernote64.png"));
    setWindowTitle("Insert URL");

    QVBoxLayout* layout = new QVBoxLayout(this);

    url = new QLineEdit(this);
    url->setText(clip);
    url->setFocus();
    url->setMinimumWidth(500);
    url->setPlaceholderText("http://");
    connect(url, SIGNAL(textChanged(QString)), this, SLOT(validateURL(QString)));
    layout->addWidget(url);

    info = new QLabel(this);
    layout->addWidget(info);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);

    okButton = buttons->button(QDialogButtonBox::Ok);
    validateURL(url->text());

    setLayout(layout);
}

void URLDialog::validateURL(QString link)
{
    QUrl u(link);

    if (u.isValid() && alowedSchemas.contains(u.scheme()) && !u.host().isEmpty()) {
        info->setText("<span style=\"color:green;\">URL is valid!</span>");
        okButton->setDisabled(false);
    } else {
        info->setText("<span style=\"color:red;\">URL is not valid!</span>");
        okButton->setDisabled(true);
    }
}

QString URLDialog::getURL()
{
    return url->text();
}

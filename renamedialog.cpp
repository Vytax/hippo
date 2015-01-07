#include "renamedialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QMessageBox>

RenameDialog::RenameDialog(QWidget *parent) :
    QDialog(parent)
{
    QVBoxLayout* layout = new QVBoxLayout(this);

    label = new QLabel(this);
    layout->addWidget(label);

    edit = new QLineEdit(this);
    edit->setMinimumWidth(250);
    edit->setMaxLength(100);
    layout->addWidget(edit);
    connect(edit, SIGNAL(textChanged(QString)), this, SLOT(textChanged(QString)));

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(validate()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);
    ok = buttons->button(QDialogButtonBox::Ok);
    ok->setDisabled(true);

    setLayout(layout);
}

void RenameDialog::setText(QString text) {
    edit->setText(text);
}

void RenameDialog::textChanged(QString text) {
    ok->setDisabled(text.isEmpty());
}

QString RenameDialog::getText() {
    return edit->text();
}

void RenameDialog::setLabel(QString text) {
    label->setText(text);
}

void RenameDialog::setBlocked(QStringList blocked, QString msg) {
    blockedWords = blocked;
    blockMsg = msg;
}

void RenameDialog::validate() {
    if (blockedWords.isEmpty()){
        accept();
        return;
    }
    if (blockedWords.contains(edit->text(), Qt::CaseInsensitive)) {
        QMessageBox msgBox(this);
        msgBox.setText(blockMsg);
        msgBox.setWindowTitle("Warning");
        msgBox.setIcon(QMessageBox::Warning);
        msgBox.exec();
        return;
    }
    accept();
}

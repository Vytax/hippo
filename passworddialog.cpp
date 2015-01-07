#include "passworddialog.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>
#include <QDebug>

PasswordDialog::PasswordDialog(QWidget *parent, bool newPassword) :
    QDialog(parent)
{
    qDebug() << "PasswordDialog()" << newPassword;

    setWindowTitle("Show encrypted text");
    QVBoxLayout* layout = new QVBoxLayout(this);

    p_error = new QLabel("<span style=\"font-weight: bold; color: red;\">Error: Wrong key!<span>", this);
    p_error->setVisible(false);
    layout->addWidget(p_error);

    QString text;
    if (newPassword)
        text = "Enter new password:";
    else
        text = "Enter passphrase to view content:";

    layout->addWidget(new QLabel(text, this));

    password = new QLineEdit(this);
    password->setEchoMode(QLineEdit::Password);
    password->setMaxLength(64);
    layout->addWidget(password);
    connect(password, SIGNAL(textChanged(QString)), this, SLOT(passChanged(QString)));

    hint = NULL;
    password2 = NULL;


    if (newPassword) {
        layout->addWidget(new QLabel("Retype new password:", this));
        password2 = new QLineEdit(this);
        password2->setEchoMode(QLineEdit::Password);
        password2->setMaxLength(64);
        layout->addWidget(password2);
        connect(password2, SIGNAL(textChanged(QString)), this, SLOT(passChanged(QString)));

        layout->addSpacing(30);
        layout->addWidget(new QLabel("Enter new password hint:", this));

        hint = new QLineEdit(this);
        hint->setMaxLength(64);
        layout->addWidget(hint);
    }

    p_hint = new QLabel(this);
    p_hint->setVisible(false);
    layout->addWidget(p_hint);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, this);
    connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
    layout->addWidget(buttons);
    ok = buttons->button(QDialogButtonBox::Ok);
    ok->setDisabled(true);

    setLayout(layout);
}

void PasswordDialog::setHint(QString hint)
{
    if (hint.isEmpty())
        return;
    p_hint->setText("Hint: " + hint);
    p_hint->show();
}

QString PasswordDialog::getPassword()
{
    return password->text();
}

QString PasswordDialog::getHint()
{
    if (hint != NULL)
        return hint->text();

    return "";
}

void PasswordDialog::setError(bool state) {
    p_error->setVisible(state);
}

void PasswordDialog::passChanged(QString text) {
    ok->setDisabled(text.isEmpty());

    if (password2 == NULL)
        return;

    if (password->text() != password2->text())
        ok->setDisabled(true);
}

#ifndef PASSWORDDIALOG_H
#define PASSWORDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>

class PasswordDialog : public QDialog
{
    Q_OBJECT
public:
    explicit PasswordDialog(QWidget *parent = 0, bool newPassword = false);

    void setHint(QString hint);
    QString getPassword();
    QString getHint();
    void setError(bool state);

private slots:
    void passChanged(QString text);

private:
    QLineEdit *password;
    QLineEdit *password2;
    QLineEdit *hint;
    QLabel *p_error;
    QLabel *p_hint;
    QPushButton *ok;
};

#endif // PASSWORDDIALOG_H

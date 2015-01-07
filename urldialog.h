#ifndef URLDIALOG_H
#define URLDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QStringList>
#include <QLabel>
#include <QPushButton>

class URLDialog : public QDialog
{
    Q_OBJECT
public:
    explicit URLDialog(QWidget *parent = 0);
    QString getURL();
    
private slots:
    void validateURL(QString link);

private:
    QLineEdit *url;
    QLabel *info;
    QPushButton *okButton;
    QStringList alowedSchemas;
    
};

#endif // URLDIALOG_H

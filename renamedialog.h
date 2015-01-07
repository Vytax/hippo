#ifndef RENAMEDIALOG_H
#define RENAMEDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

class RenameDialog : public QDialog
{
    Q_OBJECT
public:
    explicit RenameDialog(QWidget *parent = 0);

    void setText(QString text);
    QString getText();
    void setLabel(QString text);
    void setBlocked(QStringList blocked, QString msg);

signals:

private slots:
    void textChanged(QString text);
    void validate();

private:
    QLineEdit *edit;
    QPushButton *ok;
    QLabel *label;

    QStringList blockedWords;
    QString blockMsg;
};

#endif // RENAMEDIALOG_H

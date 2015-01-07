#ifndef USERINFO_H
#define USERINFO_H

#include <QDialog>

namespace Ui {
class UserInfo;
}

class UserInfo : public QDialog
{
    Q_OBJECT
    
public:
    explicit UserInfo(QWidget *parent = 0);
    ~UserInfo();

private:
    Ui::UserInfo *ui;

    QString size_human(qint64 size);
};

#endif // USERINFO_H

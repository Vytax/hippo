#ifndef OAUTH_H
#define OAUTH_H

#include <QDialog>
#include <QUrl>

typedef QHash<QString, QString> Arr;

class oauth : public QDialog
{
    Q_OBJECT
public:
    explicit oauth(QWidget *parent = 0);

    QString getParam(QString key);
    
signals:
    void verifierRetrieved(QString token, QString verifier);
public slots:
    void urlChange(QUrl url);
    void getCredentials(QString token, QString verifier);

private:
    Arr parseReply(QString data);
    QString queryUrl;
    Arr data;

    
};

#endif // OAUTH_H

#ifndef SQL_H
#define SQL_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QVariant>

QString newGUID(QString table = "", QString column = "");
QString AddSQLArray(int count);
void BindSQLArray(QSqlQuery &query, QStringList values);

class sql : public QObject
{
    Q_OBJECT
public:
    sql(QObject *parent = 0);
    ~sql();

    bool test();

    static void updateSyncStatus(QString key, QVariant value);
    static QVariant readSyncStatus(QString key, QVariant defaultValue = QVariant());

signals:

public slots:
    void dropTables();
    void checkTables();
private:
    QSqlDatabase db;

    void migrate();

};

#endif // SQL_H

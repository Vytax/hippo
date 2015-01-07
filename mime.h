#ifndef MIME_H
#define MIME_H

#include <QObject>
#include <QTimer>
#include <QCache>
#include "freedesktopmime.h"

class Mime : public QObject
{
    Q_OBJECT
public:
    explicit Mime(QObject *parent = 0);
    ~Mime();

    static Mime* m_Instance;
    static Mime* GetInstance();
    static void deleteInstance();

    QString getMime(QString file);

signals:

private slots:
    void clearReader();

private:
    QFreeDesktopMime *m;
    QFreeDesktopMime *getMimeReader();

    QTimer *timer;
    QCache<QString, QString> cache;

};

#endif // MIME_H

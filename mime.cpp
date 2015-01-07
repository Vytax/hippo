#include "mime.h"

#include <QDebug>

Mime* Mime::m_Instance = NULL;

Mime::Mime(QObject *parent) :
    QObject(parent)
{
    m = NULL;
    timer = new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(clearReader()));

    cache.setMaxCost(10);
}

Mime::~Mime() {
    clearReader();
}

Mime* Mime::GetInstance()
{
    if ( m_Instance == NULL ) {
        m_Instance = new Mime();
    }
    return m_Instance;
}

void Mime::deleteInstance()
{
    if ( m_Instance != NULL )
        delete m_Instance;
    m_Instance = NULL;
}

QFreeDesktopMime* Mime::getMimeReader() {
    if (m == NULL) {
        m = new QFreeDesktopMime();
        timer->stop();
        timer->start(120000);
    }
    return m;
}

void Mime::clearReader() {
    if (m == NULL)
        return;
    delete m;
    m = NULL;
}

QString Mime::getMime(QString file) {
    qDebug() << "getMime()" << file;
    if (cache.contains(file))
        return cache.object(file)->simplified();

    QFreeDesktopMime *mr = getMimeReader();
    if (mr == NULL)
        return "";

    QString mime = mr->fromFile(file);
    cache.insert(file, new QString(mime));

    return mime;
}

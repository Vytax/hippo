#ifndef RECOURCEREPLY_H
#define RECOURCEREPLY_H

#include <QNetworkReply>

class RecourceReply : public QNetworkReply
{
    Q_OBJECT
public:
    RecourceReply(QObject* parent, const QNetworkRequest& request);
    ~RecourceReply();

    void abort();
    qint64 bytesAvailable() const;
    bool isSequential() const;

protected:
    qint64 readData(char *data, qint64 maxSize);


public slots:    

private:
    QString hash;
    QByteArray m_buffer;
    qint64 offset;

private slots:
     void loadDone();

};

#endif // RECOURCEREPLY_H

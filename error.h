#ifndef ERROR_H
#define ERROR_H

#include <QObject>

#include "tbinaryprotocol.h"

class Error : public QObject
{
    Q_OBJECT
public:
    explicit Error(QObject *parent = 0);

    static void readExceptions(hash data);

signals:

public slots:

private:
    static void readUserException(hash data);
    static void EDAMErrorCode(int errorCode);
    static void readSystemException(hash data);
    static void readNotFoundException(hash data);
};

#endif // ERROR_H

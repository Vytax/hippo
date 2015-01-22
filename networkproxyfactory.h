#ifndef NETWORKPROXYFACTORY_H
#define NETWORKPROXYFACTORY_H

#include <QNetworkProxyFactory>

class NetworkProxyFactory : public QNetworkProxyFactory
{
public:
    NetworkProxyFactory();

    static NetworkProxyFactory* m_Instance;
    static NetworkProxyFactory* GetInstance();
    static void deleteInstance();

    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery &query = QNetworkProxyQuery());

    void loadSettings();

private:
    QString m_proxyPreference;
    QString m_proxyType;
    QString m_hostName;
    quint16 m_port;
    QString m_username;
    QString m_password;

    bool m_useDifferentProxyForHttps;
    QString m_httpsHostName;
    quint16 m_httpsPort;
    QString m_httpsUsername;
    QString m_httpsPassword;
};

#endif // NETWORKPROXYFACTORY_H

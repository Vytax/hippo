#include "networkproxyfactory.h"
#include "sql.h"

NetworkProxyFactory* NetworkProxyFactory::m_Instance = NULL;

NetworkProxyFactory::NetworkProxyFactory()
{
}

NetworkProxyFactory* NetworkProxyFactory::GetInstance()
{
    if ( m_Instance == NULL ) {
        m_Instance = new NetworkProxyFactory();
    }
    return m_Instance;
}

void NetworkProxyFactory::deleteInstance()
{
    if ( m_Instance != NULL )
        delete m_Instance;
    m_Instance = NULL;
}

void NetworkProxyFactory::loadSettings()
{
    m_proxyPreference = sql::readSyncStatus("proxyPreference", "system").toString();
    m_proxyType = sql::readSyncStatus("proxyType", "HTTP").toString();
    m_hostName = sql::readSyncStatus("proxyHostName", "").toString();
    m_port = sql::readSyncStatus("proxyPort", "80").toInt();
    m_username = sql::readSyncStatus("proxyUserName", "").toString();
    m_password = sql::readSyncStatus("proxyPassword", "").toString();

    m_useDifferentProxyForHttps = sql::readSyncStatus("useDifferentProxyForHttps", false).toBool();
    m_httpsHostName = sql::readSyncStatus("proxyHttpsHostName", "").toString();
    m_httpsPort = sql::readSyncStatus("proxyHttpsPort", "443").toInt();
    m_httpsUsername = sql::readSyncStatus("proxyHttpsUsername", "").toString();
    m_httpsPassword = sql::readSyncStatus("proxyHttpsPassword", "").toString();

}

QList<QNetworkProxy> NetworkProxyFactory::queryProxy(const QNetworkProxyQuery &query)
{
    QList<QNetworkProxy> proxyList;

    m_proxyPreference = m_proxyPreference.toLower();
    if (m_proxyPreference == "system") {
        proxyList.append(systemProxyForQuery(query));
    }
    else if (m_proxyPreference == "manual") {
        QNetworkProxy proxy;
        if (m_proxyType == "HTTP")
            proxy = QNetworkProxy(QNetworkProxy::HttpProxy);
        else
            proxy = QNetworkProxy(QNetworkProxy::Socks5Proxy);

        if (m_useDifferentProxyForHttps && query.protocolTag() == "https") {
            proxy.setHostName(m_httpsHostName);
            proxy.setPort(m_httpsPort);
            proxy.setUser(m_httpsUsername);
            proxy.setPassword(m_httpsPassword);
        }
        else {
            proxy.setHostName(m_hostName);
            proxy.setPort(m_port);
            proxy.setUser(m_username);
            proxy.setPassword(m_password);
        }

        if (proxy.hostName().isEmpty()) {
            proxy = QNetworkProxy::NoProxy;
        }

        proxyList.append(proxy);
    }
    else
        proxyList.append(QNetworkProxy::NoProxy);

    return proxyList;
}

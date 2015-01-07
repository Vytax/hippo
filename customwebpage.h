#ifndef CUSTOMWEBPAGE_H
#define CUSTOMWEBPAGE_H

#include <QWebPage>
#include <QObject>

class CustomWebPage : public QWebPage
{
    Q_OBJECT
public:
    explicit CustomWebPage(QObject *parent = 0);
protected:
    void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID);
    void javaScriptAlert ( QWebFrame * frame, const QString & msg );

signals:

public slots:

};

#endif // CUSTOMWEBPAGE_H

#include <QDebug>

#include "customwebpage.h"

CustomWebPage::CustomWebPage(QObject *parent) :
    QWebPage(parent)
{
}

void CustomWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID){
   QString logEntry = message +" on line:"+ QString::number(lineNumber) +" Source:"+ sourceID;
   qDebug()<<logEntry;
}

void CustomWebPage::javaScriptAlert ( QWebFrame * frame, const QString & msg ) {
    qDebug() << "JS Error: " << msg;
}

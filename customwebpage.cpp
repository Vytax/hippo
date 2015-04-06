#include "Logger.h"
#include "customwebpage.h"

CustomWebPage::CustomWebPage(QObject *parent) :
    QWebPage(parent)
{
}

void CustomWebPage::javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID){
   QString logEntry = message +" on line:"+ QString::number(lineNumber) +" Source:"+ sourceID;
   LOG_WARNING(logEntry);
}

void CustomWebPage::javaScriptAlert ( QWebFrame * frame, const QString & msg ) {
    Q_UNUSED(frame);
    LOG_ERROR(msg);
}

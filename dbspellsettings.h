#ifndef DBSPELLSETTINGS_H
#define DBSPELLSETTINGS_H

#include "3rdparty/spellcheck/speller.h"

#include <QWebView>

class DBSpellSettings: public SpellSettings
{
public:
    DBSpellSettings(QObject* parent = 0, QWebView *editor = 0);

    bool isEnabled();
    void setEnabled(bool value);
    QString language();
    void setLanguage(QString lang);

private:
    QWebView *m_editor;
};

#endif // DBSPELLSETTINGS_H

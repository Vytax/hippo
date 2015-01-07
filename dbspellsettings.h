#ifndef DBSPELLSETTINGS_H
#define DBSPELLSETTINGS_H

#include "3rdparty/spellcheck/speller.h"

class DBSpellSettings: public SpellSettings
{
public:
    DBSpellSettings(QObject* parent = 0);

    bool isEnabled();
    void setEnabled(bool value);
    QString language();
    void setLanguage(QString lang);
};

#endif // DBSPELLSETTINGS_H

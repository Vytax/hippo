#include "dbspellsettings.h"
#include "sql.h"

#include <QDebug>

DBSpellSettings::DBSpellSettings(QObject *parent) : SpellSettings(parent)
{
}

bool DBSpellSettings::isEnabled() {

    qDebug() << "DBSpellSettings::isEnabled()";
    return sql::readSyncStatus("spellingEnabled", true).toBool();
}

void DBSpellSettings::setEnabled(bool value) {
    sql::updateSyncStatus("spellingEnabled", value);
}

QString DBSpellSettings::language() {
    return sql::readSyncStatus("spellingLanguage", QLocale::system().name()).toString();
}

void DBSpellSettings::setLanguage(QString lang) {
    sql::updateSyncStatus("spellingLanguage", lang);
}

#include "dbspellsettings.h"
#include "sql.h"

#include <QWebFrame>
#include <QLocale>

DBSpellSettings::DBSpellSettings(QObject *parent, QWebView *editor) : SpellSettings(parent), m_editor(editor)
{
}

bool DBSpellSettings::isEnabled() {

    if (m_editor)
        if (!m_editor->page()->mainFrame()->evaluateJavaScript("editMode").toBool())
            return false;

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

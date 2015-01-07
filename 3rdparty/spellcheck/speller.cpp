/* ============================================================
* qtwebkit-spellcheck Spell checking plugin using Hunspell
* Copyright (C) 2013  David Rosca <nowrep@gmail.com>
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* ============================================================ */
#include "speller.h"

#include <QStringList>
#include <QTextCodec>
#include <QTextStream>
#include <QFile>
#include <QRegExp>
#include <QDir>
#include <QDirIterator>
#include <QDebug>

#include <hunspell/hunspell.hxx>

Speller* Speller::m_Instance = NULL;
SpellSettings* Speller::m_settings = NULL;

SpellSettings::SpellSettings(QObject *parent): QObject(parent) {

}

Speller::Speller(QObject *parent): QObject(parent)
{
    qDebug() << "new Speller()";
    s_hunspell = NULL;
    s_codec = 0;

    if (m_settings)
        m_enabled = m_settings->isEnabled();
    else
        m_enabled = false;
}

bool Speller::initialize()
{
    if (initialized())
        return true;

    m_dictionaryPath = getDictionaryPath();

    QString defLanguage;

    if (m_settings)
        defLanguage = m_settings->language();
    else
        defLanguage = QLocale::system().name();

    if (!availableLanguages().contains(defLanguage))
        defLanguage = availableLanguages().keys().first();

    loadLanguage(defLanguage);

    if (!m_settings)
        m_enabled = true;

    return initialized();
}

void Speller::loadLanguage(QString lang) {

    if (lang.isEmpty() || !availableLanguages().contains(lang))
        return;

    if (initialized() && (s_language == lang))
        return;

    QString dicPath = m_dictionaryPath + lang + ".dic";
    QString affPath = m_dictionaryPath + lang + ".aff";

    if (!QFile(dicPath).exists() || !QFile(affPath).exists()) {
        qWarning() << "SpellCheck: Initialization failed!";
        return;
    }

    if (initialized()) {
        delete s_hunspell;
        s_language = "";
        s_codec = 0;
    }

    s_hunspell = new Hunspell(affPath.toLocal8Bit().constData(),
                              dicPath .toLocal8Bit().constData());

    s_codec = QTextCodec::codecForName(s_hunspell->get_dic_encoding());
    s_language = lang;

    qDebug() << "SpellCheck: Language =" << language();
}

void Speller::setLanguage(QString lang) {
    if (lang.isEmpty())
        return;

    loadLanguage(lang);
    if (m_settings)
        m_settings->setLanguage(lang);
}

bool Speller::initialized() {
    return s_hunspell != NULL;
}

bool Speller::isEnabled() {
    return m_enabled && initialized();
}

void Speller::setEnabled(bool enabled) {
    m_enabled = enabled;
    if (m_settings)
        m_settings->setEnabled(enabled);
}

Speller* Speller::GetInstance()
{
    if ( m_Instance == NULL ) {
        m_Instance = new Speller();
    }

    if (!m_Instance->initialize()) {
        delete m_Instance;
        m_Instance = NULL;
    }

    return m_Instance;
}

void Speller::deleteInstance()
{
    if ( m_Instance != NULL )
        delete m_Instance;
    m_Instance = NULL;
}

QString Speller::backend() const
{
    return QString("Hunspell");
}

QString Speller::language() const
{
    return s_language;
}

void Speller::learnWord(const QString &word)
{
    const char* encodedWord = s_codec->fromUnicode(word).constData();
    s_hunspell->add(encodedWord);
}

void Speller::ignoreWordInSpellDocument(const QString &word)
{
    m_ignoredWords.append(word);
}

bool Speller::isMisspelled(const QString &string)
{
    if (m_ignoredWords.contains(string)) {
        return false;
    }

    const char* encodedString = s_codec->fromUnicode(string).constData();
    return s_hunspell->spell(encodedString) == 0;
}

QStringList Speller::suggest(const QString &word)
{
    char **suggestions;
    const char* encodedWord = s_codec->fromUnicode(word).constData();
    int count = s_hunspell->suggest(&suggestions, encodedWord);

    QStringList suggests;
    for(int i = 0; i < count; ++i) {
        suggests.append(s_codec->toUnicode(suggestions[i]));
    }
    s_hunspell->free_list(&suggestions, count);

    return suggests;
}

QString Speller::getDictionaryPath()
{
    QString defaultDicPath = "/usr/share/hunspell/";
    if (!QDir(defaultDicPath).exists())
        defaultDicPath = "/usr/share/myspell/";
    if (!QDir(defaultDicPath).exists())
        defaultDicPath = "";

    QString dicPath = QString::fromLocal8Bit(qgetenv("DICPATH"));
    if (!dicPath.isEmpty() && !dicPath.endsWith(QDir::separator())) {
            dicPath.append(QDir::separator());
    }

    if (!dicPath.isEmpty() && QDir(dicPath).exists()) {
        return dicPath;
    }

    return defaultDicPath;
}

Speller::~Speller()
{
    delete s_hunspell;
}

QHash<QString, QString> Speller::availableLanguages()
{
    if (!m_availableLanguages.isEmpty()) {
        return m_availableLanguages;
    }

    QDirIterator it(m_dictionaryPath, QStringList("*.dic"), QDir::Files);

    while (it.hasNext()) {
        const QString affFilePath = it.next().replace(QLatin1String(".dic"), QLatin1String(".aff"));

        if (!QFile(affFilePath).exists()) {
            continue;
        }

        QString code = it.fileInfo().baseName();
        QString name = nameForLanguage(code);

        m_availableLanguages[code] = name;
    }

    return m_availableLanguages;
}

QString Speller::nameForLanguage(const QString &code) const
{
    QLocale loc = QLocale(code);
    QString name = QLocale::languageToString(loc.language());

    if (loc.country() != QLocale::AnyCountry) {
        name.append(" / " + loc.nativeLanguageName());
    }

    return name;
}

void Speller::setSettings(SpellSettings *settings)
{
    m_settings = settings;
}

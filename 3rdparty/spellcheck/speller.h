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
#ifndef SPELLER_H
#define SPELLER_H

#include <QStringList>
#include <QHash>
#include <QVariant>

class QTextCodec;
class Hunspell;

class SpellSettings: public QObject
{
    Q_OBJECT
public:
    SpellSettings(QObject* parent = 0);
    virtual bool isEnabled() = 0;
    virtual void setEnabled(bool value) = 0;
    virtual QString language() = 0;
    virtual void setLanguage(QString lang) = 0;
};

class Speller: public QObject
{
    Q_OBJECT
public:

    explicit Speller(QObject* parent = 0);
    ~Speller();

    static Speller* GetInstance();
    static void deleteInstance();
    static void setSettings(SpellSettings *settings);

    bool initialize();
    void setLanguage(QString lang);
    bool initialized();

    bool isEnabled();

    QString backend() const;
    QString language() const;

    void learnWord(const QString &word);
    void ignoreWordInSpellDocument(const QString &word);

    bool isMisspelled(const QString &string);
    QStringList suggest(const QString &word);
    QHash<QString, QString> availableLanguages();

public slots:
    void setEnabled(bool enabled);

private:
    void loadLanguage(QString lang);
    QString getDictionaryPath();
    QString nameForLanguage(const QString &code) const;

    static Speller* m_Instance;
    Hunspell* s_hunspell;
    QTextCodec* s_codec;
    QString m_dictionaryPath;
    QString s_language;
    bool m_enabled;

    static SpellSettings *m_settings;

    QStringList m_ignoredWords;
    QHash<QString, QString> m_availableLanguages;
};

#endif // SPELLER_H

#ifndef ENML2_H
#define ENML2_H

#include <QObject>
#include <QDomDocument>
#include <QHash>
#include <QStringList>

class enml2 : public QObject
{
    Q_OBJECT
public:
    explicit enml2(QObject *parent = 0);

    Q_INVOKABLE QString toString();
    Q_INVOKABLE bool openNewTag(QString name);
    Q_INVOKABLE bool validRootAttribute(QString attr);

public slots:
    void createDoc();
    void writeText(QString text);
    void closeLastTag();
    void setAttribute(QString name, QString value);
    void cleanEmptyTags();

private:
    QDomDocument doc;
    QDomNode currentNode;

    QHash<QString, QStringList> enml2Tags;

    QString currentTagName();
    QDomNode root();
    void clean(QDomNode node);
    bool preformated();
    QString trimmed(QString str);

};

#endif // ENML2_H

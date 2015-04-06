#include "enml2.h"

#include <QFile>

enml2::enml2(QObject *parent) :
    QObject(parent)
{

    QDomDocument doc("mydocument");
    QFile file("://scripts/enml2.xsd");
    if (!file.open(QIODevice::ReadOnly))
        return;
    if (!doc.setContent(&file)) {
        file.close();
        return;
    }
    file.close();

    QDomElement docElem = doc.documentElement();

    QDomNode n = docElem.firstChild();
    while(!n.isNull()) {
        QDomElement e = n.toElement();
        if(!e.isNull() && e.hasAttribute("name")) {

            QString name = e.attribute("name");
            QStringList attrList;

            QDomNodeList complexType = e.elementsByTagName("complexType");

            if (!complexType.isEmpty()) {
                QDomElement ea = complexType.at(0).toElement();

                if (!ea.isNull()) {
                    QDomNodeList attributes = ea.elementsByTagName("attribute");

                    for (int i = 0; i < attributes.count(); i++) {
                        QDomElement attr = attributes.at(i).toElement();

                        if(!attr.isNull() && attr.hasAttribute("name")) {
                            attrList.append(attr.attribute("name"));
                        }
                    }
                }
            }
            enml2Tags[name] = attrList;
        }
        n = n.nextSibling();
    }
}

void enml2::createDoc() {

    doc.clear();

    QDomImplementation impl;
    QDomDocumentType dtd = impl.createDocumentType("en-note", QString(), "http://xml.evernote.com/pub/enml2.dtd");
    doc = impl.createDocument(0, "en-note", dtd);

    QDomNode root = doc.firstChild();

    QDomNode node( doc.createProcessingInstruction( "xml", "version=\"1.0\" encoding=\"UTF-8\"" ) );
    doc.insertBefore( node, root );

    currentNode = root;
}

QString enml2::toString() {
    return doc.toString(-1);
}

void enml2::writeText(QString text) {
    QString str = text;

    if (!preformated())
        str = trimmed(str);

    if (str.length() == 0)
        return;

    currentNode.appendChild(doc.createTextNode(str));
}

bool enml2::openNewTag(QString name) {
    if (!enml2Tags.contains(name))
        return false;

    currentNode = currentNode.appendChild(doc.createElement(name));
    return true;
}

bool enml2::validRootAttribute(QString attr) {
    return enml2Tags["en-note"].contains(attr, Qt::CaseInsensitive);
}

void enml2::closeLastTag() {
    if (!currentNode.parentNode().isNull() && (currentNode.nodeName() != "en-note"))
        currentNode = currentNode.parentNode();
}

void enml2::setAttribute(QString name, QString value) {
    if (!enml2Tags[currentTagName()].contains(name, Qt::CaseInsensitive))
        return;

    currentNode.toElement().setAttribute(name, value);
}

QString enml2::currentTagName() {
    return currentNode.nodeName();
}

QDomNode enml2::root() {
    return doc.elementsByTagName("en-note").at(0);
}

void enml2::cleanEmptyTags() {
    clean(root());
}

void enml2::clean(QDomNode node) {
    if (node.isNull())
        return;

    QDomNodeList childNodes = node.childNodes();
    int childCount = childNodes.count();

    QStringList tagTypes;
    tagTypes << "div" << "span";

    for (int i = 0; i < childCount; i++) {
        QDomNode child = childNodes.at(i);
        clean(child);
        if (tagTypes.contains(child.nodeName(), Qt::CaseInsensitive))
            if (child.childNodes().count() == 0)
                node.removeChild(child);
    }
}

bool enml2::preformated() {
    QStringList preElements;
    preElements << "pre";
    preElements << "code";

    QDomNode node = currentNode;
    while (!node.isNull()) {
        if (preElements.contains(node.nodeName(), Qt::CaseInsensitive))
            return true;
        node = node.parentNode();
    }

    return false;
}

QString enml2::trimmed(QString str) {
    QString t = str.trimmed();
    if (t.isEmpty())
        return t;

    QString result;

    if (str.at(0).isSpace())
        result = " ";

    result += t;

    if (str.at(str.size()-1).isSpace())
        result += " ";

    return result;
}

#ifndef TAG_H
#define TAG_H

#include "tbinaryprotocol.h"

#include <QObject>

/*   A tag within a user's account is a unique name which may be organized
   a simple hierarchy.
  <dl>
   <dt>guid</dt>
     <dd>The unique identifier of this tag. Will be set by the service,
     so may be omitted by the client when creating the Tag.
     <br/>
     Length:  EDAM_GUID_LEN_MIN - EDAM_GUID_LEN_MAX
     <br/>
     Regex:  EDAM_GUID_REGEX
     </dd>

   <dt>name</dt>
     <dd>A sequence of characters representing the tag's identifier.
     Case is preserved, but is ignored for comparisons.
     This means that an account may only have one tag with a given name, via
     case-insensitive comparison, so an account may not have both "food" and
     "Food" tags.
     May not contain a comma (','), and may not begin or end with a space.
     <br/>
     Length:  EDAM_TAG_NAME_LEN_MIN - EDAM_TAG_NAME_LEN_MAX
     <br/>
     Regex:  EDAM_TAG_NAME_REGEX
     </dd>

   <dt>parentGuid</dt>
     <dd>If this is set, then this is the GUID of the tag that
     holds this tag within the tag organizational heirarchy.  If this is
     not set, then the tag has no parent and it is a "top level" tag.
     Cycles are not allowed (e.g. a->parent->parent == a) and will be
     rejected by the service.
     <br/>
     Length:  EDAM_GUID_LEN_MIN - EDAM_GUID_LEN_MAX
     <br/>
     Regex:  EDAM_GUID_REGEX
     </dd>

   <dt>updateSequenceNum</dt>
     <dd>A number identifying the last transaction to
     modify the state of this object.  The USN values are sequential within an
     account, and can be used to compare the order of modifications within the
     service.
     </dd>
   </dl>

  Attributes:
   - guid
   - name
   - parentGuid
   - updateSequenceNum
   */

class Tag : public QObject
{
Q_OBJECT
public:
    Tag(QObject *parent = 0);
    void toSQL();
    bool loadFromSQL(QString id);
    void loadFromData(hash data);

    enum TagField {
        T_NEW = 0,
        T_GUID = 1,
        T_NAME = 2,
        T_PARENT = 3
    };

    void editField(int field);
    static void editField(QString id, int field);

    typedef QHash<TagField, QVariant> TagUpdates;
    void update(TagUpdates updates);
    void sync();
    QString createNewTag(QString tagName);

    static void expungeTag(QString id);

private:
    QString guid;
    QString name;
    QString parentGuid;
    qint32 updateSequenceNum;

    QByteArray createPushContentPost();

};

#endif // TAG_H

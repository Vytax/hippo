var contentModified = false;
var imageTypes = ['image/jpeg','image/png','image/gif'];
var editMode = false;
var title = '';
var oldTitle = '';
var titleModified = false;
var guid = '';
var active = true;
var spellRange = false;
var specialTags = ['en-media', 'en-crypt', 'en-todo'];
var breakTags = ['br', 'p', 'div', 'h1', 'h2', 'h3', 'li'];
var versions = {};

jsB.encryptText.connect(encrypt);
jsB.insertToDo.connect(insertToDo);
jsB.titleChanged.connect(modifyTitle);

function setup() {
    $('body').observe('input', input);    
}

function isOnEditableArea() {

    var range, sel, container, node;
    if (window.getSelection) {

        sel = window.getSelection();
        if (sel.rangeCount > 0) {
            range = sel.getRangeAt(0);
        }

        if (range) {
            container = range.commonAncestorContainer;

            // Check if the container is a text node and return its parent if so
            node = container.nodeType === 3 ? container.parentNode : container;
        }
        if (node) return node.isContentEditable;
        return false;
    }
}
function input(event) {
    if (!editMode)
        return;
    contentModified = true;
}
function setEndOfContenteditable()
{
    var range,selection;
    range = document.createRange();
    range.selectNodeContents($('body'));
    range.collapse(false);
    selection = window.getSelection();
    selection.removeAllRanges();
    selection.addRange(range);
}
function setEditable(editable)
{
    editMode = editable;
    $('body').setAttribute('contenteditable', editable);
    if (editable)
        setEndOfContenteditable();

    items = $$('.normal_mode');
    items.each(function(item) {
        if (editable)
            item.hide();
        else
            item.show();
    });
    items = $$('.edit_mode');
    items.each(function(item) {
        if (editable)
            item.show();
        else
            item.hide();
    });
    items = $$('.draggable');
    items.each(function(item) {
        item.setAttribute('draggable', !editable);
    });
}
function loadEnMedia(item)
{
    var type = item.readAttribute('type').toLowerCase();
    item.setAttribute('contenteditable', false);

    if (type === 'application/pdf')
        loadPDF(item);
    else if (imageTypes.indexOf(type) >= 0)
        loadImage(item);
    else if (type === 'application/octet-stream')
        loadFile(item);
}
function loadPDF(item)
{
    var id = item.readAttribute('hash');
    item.update();

    var pdfArea = new Element('div', {'class': 'pdfarea', 'contenteditable': 'false', 'id': id});
    item.appendChild(pdfArea);

    var pdfIco = new Element('div', {'class': 'pdfico draggable'});
    pdfIco.hash = id;
    pdfArea.appendChild(pdfIco);
    pdfIco.observe("dragstart", dragStart.bind(pdfIco));

    var fileName = jsB.getResourceFileName(id);
    title = '<b>Embedded PDF Document</b><br />';
    if (!fileName.blank())
        title = title + ' (' + fileName + ')';

    pdfArea.appendChild(new Element('div', {'class': 'pdftitle'}).update(title));

    var menu = new Element('div', {'class': 'menu normal_mode'});
    pdfArea.appendChild(menu);
    if (editMode)
        menu.hide();

    var preview = new Element('span', {'hint': 'Preview file', 'type': 'preview'}).update('Preview');
    setClickEvent(preview);
    setHintEvent(preview);
    menu.appendChild(preview);

    var saveas = new Element('span', {'hint': 'Save file to a disk', 'type': 'saveas'}).update('Save As...');
    setClickEvent(saveas);
    setHintEvent(saveas);
    menu.appendChild(saveas);

    var open = new Element('span', {'hint': 'Open in external PDF viewer', 'type': 'open'}).update('Open...');
    setClickEvent(open);
    setHintEvent(open);
    menu.appendChild(open);

    var menu2 = new Element('div', {'class': 'menu edit_mode'});
    pdfArea.appendChild(menu2);
    if (!editMode)
        menu2.hide();

    var del = new Element('span', {'hint': 'Delete file', 'type': 'delete'}).update('Delete');
    setClickEvent(del);
    setHintEvent(del);
    menu2.appendChild(del);
}
function loadFile(item) {

    var id = item.readAttribute('hash');
    item.update();

    var fileArea = new Element('div', {'class': 'filearea', 'contenteditable': 'false', 'id': id});
    item.appendChild(fileArea);

    var fileico = new Element('div', {'class': 'fileico draggable'});
    fileico.hash = id;
    fileArea.appendChild(fileico);
    fileico.observe("dragstart", dragStart.bind(fileico));

    var fileName = jsB.getResourceFileName(id);
    var title = '<b>Embedded File</b><br />';
    if (!fileName.blank())
        title = title + ' (' + fileName + ')';

    fileArea.appendChild(new Element('div', {'class': 'filetitle'}).update(title));

    var menu = new Element('div', {'class': 'menu normal_mode'});
    fileArea.appendChild(menu);
    if (editMode)
        menu.hide();

    if (!fileName.empty()) {
        var open = new Element('span', {'hint': 'Open...', 'type': 'open'}).update('Open...');
        setClickEvent(open);
        setHintEvent(open);
        menu.appendChild(open);
    }

    var saveas = new Element('span', {'hint': 'Save file to a disk', 'type': 'saveas'}).update('Save As...');
    setClickEvent(saveas);
    setHintEvent(saveas);
    menu.appendChild(saveas);

    var menu2 = new Element('div', {'class': 'menu edit_mode'});
    fileArea.appendChild(menu2);
    if (!editMode)
        menu2.hide();

    var del = new Element('span', {'hint': 'Delete file', 'type': 'delete'}).update('Delete');
    setClickEvent(del);
    setHintEvent(del);
    menu2.appendChild(del);

}
function setHintEvent(item) {
    item.observe('mouseover', function(event) {
        jsB.hintMessage(this.readAttribute('hint'), 5000);
    });
}
function setClickEvent(item) {
    item.observe('click', function(event) {
        enMedia = this.up('en-media');
        hash = enMedia.readAttribute('hash');
        if (hash.blank())
            return;
        type = item.readAttribute('type');
        if (type == 'preview')
            img = new PDFBrowser(hash);
        else if (type == 'saveas')
            jsB.saveAsResource(hash);
        else if (type == 'open')
            jsB.externalOpenResource(hash);
        else if (type == 'delete') {
            enMedia.remove();
            contentModified = true;
            items = $$('en-media[hash=' + hash + ']');
            if (items.size() < 1) {
                jsB.deleteResource(hash, guid);
            }
        }
    });
}
function insertPDF(hash) {
    document.execCommand('insertHTML',false,'<en-media hash="' + hash + '" id="new_pdf_' + hash + '" type="application/pdf"></en-media>');
    item = $('new_pdf_' + hash);
    item.removeAttribute('id');
    loadPDF(item);
}
function dragStart(evt) {
    evt.preventDefault();
    jsB.dragResource(this.hash);
}
function loadImage(item) {
    var id = item.readAttribute('hash');
    item.update();

    var img = new Element('img', {'src': 'resource://' + id});
    img.hash = id;
    img.observe("dragstart", dragStart.bind(img));

    if (item.hasAttribute('width'))
        img.writeAttribute('width', item.readAttribute('width'));

    if (item.hasAttribute('height'))
        img.writeAttribute('height', item.readAttribute('height'));

    item.insertBefore(img, null);
}
function insertImage(hash, mime) {
    document.execCommand('insertHTML',false,'<en-media hash="' + hash + '" id="new_image_' + hash + '" type="' + mime + '"></en-media>');
    item = $('new_image_' + hash);
    item.removeAttribute('id');
    item.setAttribute('contenteditable', false);
    loadImage(item);
}
function loadAboutInfo() {
    if (guid === 'main')
        return;

    checkModified();

    jsB.noteChanged(' ');
    jsB.noteSelectionChanged(false);
    title = '';
    titleModified = false;
    contentModified = false;
    body = $('body');
    body.update('<div class="loading"/>');
    guid = 'main';
    var r = new Ajax.Request('qrc:///html/mainajax.html', {
          method: 'get',
          onComplete: function(response) {
              body.update(response.responseText);
              guid = 'main';
              setEditable(false);

      }
    });
}
function loadNote(note_id, editable) {
    checkModified();
    jsB.noteSelectionChanged(false);

    var data = jsB.loadNote(note_id);
    if (!data.ok)
        return;

    versions = {};
    guid = data.guid;
    active = data.active && !data.hasConflict;
    setContent(data.content);
    titleModified = false;
    contentModified = false;
    jsB.titleUpdated(guid, data.title);
    title = data.title;
    oldTitle = title;
    jsB.activeNoteSelectionChanged(active);
    jsB.noteSelectionChanged(true);

    jsB.noteChanged(guid);
    setEditable(editable);
    jsB.editingStarted(editable);

    if (data.hasConflict) {
        versions[data.contentHash] = data.content;
        versions[data.conflict.contentHash] = data.conflict.content;
        jsB.conflictAdded(data.updated, data.contentHash, true);
        jsB.conflictAdded(data.conflict.updated, data.conflict.contentHash, false);
        jsB.showConflict();
    }
}
function setContent(content) {
    body = $('body');
    body.update();

    if (content.empty())
        return;

    var parser = new DOMParser();
    var doc = parser.parseFromString(content, "application/xml");


    if (doc.documentElement.hasAttribute('style'))
        body.setAttribute('style', doc.documentElement.getAttribute('style'));

    var attrCount = body.attributes.length;
    for (var j = attrCount - 1; j >= 0; j--) {
        var attr = body.attributes[j];
        if (enmlWritter.validRootAttribute(attr.nodeName)) {
            body.removeAttributeNode(attr);
        }
    }
    attrCount = doc.documentElement.attributes.length;
    for (var j = 0; j<attrCount; j++) {
        var attr = doc.documentElement.attributes[j];
        body.setAttribute(attr.nodeName, attr.value);
    }

    body.update();
    parseENML(doc.documentElement, body);

    delete doc;

    items = $$('parsererror');
    items.each(function(item) {
        item.remove();
    });
}
function modifyTitle(newTitle, noteGuid) {
    jsB.debug('modifyTitle ' + newTitle);
    if (guid !== noteGuid)
        return;

    titleModified = true;
    title = newTitle;
}
function checkModified() {
    if (!titleModified && !contentModified)
        return;

    jsB.debug("checkModified()");

    if (title.blank()) {
        title = oldTitle;
        jsB.titleUpdated(guid, title);
    }

    var mod = $H();
    mod.set('guid', guid);

    mod.set('titleModified', titleModified);
    if (titleModified)
        mod.set('title', title.trim());

    mod.set('contentModified', contentModified);
    if (contentModified)
        mod.set('content', getENML());

    mod.set('updated', new Date().getTime());

    if (jsB.updateNote(mod.toJSON())) {
        titleModified = false;
        contentModified = false;
    }
}
function insertToDo() {
    document.execCommand('insertHTML',false,'<input type="checkbox"/>');
}
function loadToDo(item, html) {
    var value = item.getAttribute('checked');
    // true, false or absent (the same as false)
    if (value != null) {
       value = value.toLowerCase();
    } else {
       value = 'false';
    }

    var checkbox = new Element('input', {'type': 'checkbox'});
    if (value === 'true')
        checkbox.checked = true;
    if (!active)
        checkbox.writeAttribute('disabled', 'true');

    html.appendChild(checkbox);

    checkbox.observe('click', function(event){contentModified = true;});
}
function getWordAtPoint(elem, x, y) {
    if(elem.nodeType == elem.TEXT_NODE) {
        var range = elem.ownerDocument.createRange();
        range.selectNodeContents(elem);
        var currentPos = 0;
        var endPos = range.endOffset;
        while(currentPos+1 < endPos) {
            range.setStart(elem, currentPos);
            range.setEnd(elem, currentPos+1);
            if(range.getBoundingClientRect().left <= x && range.getBoundingClientRect().right  >= x &&
                    range.getBoundingClientRect().top  <= y && range.getBoundingClientRect().bottom >= y) {
                range.expand("word");
                var ret = range.toString();
                spellRange = range.cloneRange();
                range.detach();
                return(ret);
            }
            currentPos += 1;
        }
    } else {
        for(var i = 0; i < elem.childNodes.length; i++) {
            var range = elem.childNodes[i].ownerDocument.createRange();
            range.selectNodeContents(elem.childNodes[i]);
            if(range.getBoundingClientRect().left <= x && range.getBoundingClientRect().right  >= x &&
                    range.getBoundingClientRect().top  <= y && range.getBoundingClientRect().bottom >= y) {
                range.detach();
                return(getWordAtPoint(elem.childNodes[i], x, y));
            } else {
                range.detach();
            }
        }
    }
    return(null);
}
function getSpellingWord(x,y) {
    spellRange = false;

    var word = getWordAtPoint($('body'), x, y);
    var data = jsB.trimWord(word);

    var start = spellRange.startOffset;
    var end = spellRange.endOffset;

    if ((data.fromStart > 0) || (data.fromEnd > 0)) {
        start = start + data.fromStart;
        end = end - data.fromEnd
        if (start < end){
            spellRange.setStart(spellRange.commonAncestorContainer, start);
            spellRange.setEnd(spellRange.commonAncestorContainer, end);
        }
    }
    return spellRange.toString();
}
function replaceSpellWord(word) {
    if (spellRange === false)
        return;

    spellRange.deleteContents();
    var node = spellRange.createContextualFragment(word);
    spellRange.insertNode(node);

    spellRange = false;
    contentModified = true;
}
function getENML() {
    enmlWritter.createDoc();

    var body = $('body');
    var attrCount = body.attributes.length;
    for (var j = 0; j < attrCount; j++) {
        var attr = body.attributes[j];
        enmlWritter.setAttribute(attr.nodeName.toLowerCase(), attr.value);
    }

    parseHTML(body);
    enmlWritter.cleanEmptyTags();

    var str = enmlWritter.toString();
    return str;
}
function parseHTML(node) {
    var childCount = node.childNodes.length;
    for (var i = 0; i<childCount; i++) {
        var child = node.childNodes[i];
        if (child.nodeType === 3) {
            enmlWritter.writeText(child.data);
        } else if (child.nodeType === 1) {
            var nodeName = child.tagName.toLowerCase();
            if (nodeName === 'article' || nodeName === 'section' || nodeName === 'time' || nodeName === 'main' || nodeName === 'header')
                nodeName = 'div';

            if ((nodeName == 'input') && (child.type == 'checkbox')) {
                if (enmlWritter.openNewTag('en-todo')) {
                    enmlWritter.setAttribute('checked', child.checked);
                    enmlWritter.closeLastTag();
                }
            }
            else if (enmlWritter.openNewTag(nodeName)) {
                var attrCount = child.attributes.length;
                for (var j = 0; j<attrCount; j++) {
                    var attr = child.attributes[j];
                    enmlWritter.setAttribute(attr.nodeName.toLowerCase(), attr.value);
                }
                if (specialTags.indexOf(nodeName) < 0)
                    parseHTML(child);
                else if (nodeName === 'en-crypt') {
                    var h = child.down('.data');
                    if (h)
                        enmlWritter.writeText(h.innerHTML);
                }
                enmlWritter.closeLastTag();
            }
        }
    }
}
function parseENML(node, htmlNode) {
    var childCount = node.childNodes.length;
    for (var i = 0; i<childCount; i++) {
        var child = node.childNodes[i];
        if (child.nodeType === 3) {
            htmlNode.appendChild(document.createTextNode(child.data));
        } else if (child.nodeType === 1) {
            var nodeName = child.tagName.toLowerCase();
            if (nodeName === 'en-todo') {
                loadToDo(child, htmlNode);
            }
            else {

                var el = new Element(nodeName);

                var attrCount = child.attributes.length;
                for (var j = 0; j<attrCount; j++) {
                    var attr = child.attributes[j];
                    el.setAttribute(attr.nodeName, attr.value);
                }
                htmlNode.appendChild(el);

                if (specialTags.indexOf(nodeName) < 0)
                    parseENML(child, el);
                else {
                    if (nodeName === 'en-media')
                        loadEnMedia(el);
                    if (nodeName === 'en-crypt') {
                        if (child.firstChild !== null)
                            loadEnCrypt(el, child.firstChild.data);
                    }
                }
            }
        }
    }
}
function loadEnCrypt(item, content) {
    var img = new Element('img', {'src': 'qrc:///img/encrypted_text_button.gif', 'hint': 'Encrypted content', 'class': 'cryptButton pointer'});
    setHintEvent(img);
    item.appendChild(img);
    img.observe("dragstart", function(event) {event.preventDefault();});

    var data = new Element('div', {'class' : 'data'});
    data.hide();
    data.update(content);
    item.appendChild(data);

    item.setAttribute('contenteditable', false);

    img.observe('click', function(event) {
        var encrypt = this.up('en-crypt');
        var hint = encrypt.readAttribute('hint');
        var data = encrypt.down('.data').innerHTML;

        var html = jsB.decrypt(data, hint);
        if (!html.empty()) {
            this.hide();

            var main = new Element('div', {'class': 'encrypted'});
            encrypt.appendChild(main);

            var cont = new Element('div', {'class': 'content'});
            cont.update(html);
            main.appendChild(cont);

            var menu = new Element('div', {'class': 'menu normal_mode'});
            main.appendChild(menu);
            if (editMode)
                menu.hide();

            var hide = new Element('span', {'hint': 'Hide encrypted content'}).update('Hide');
            setHintEvent(hide);
            hide.observe('click', function(event) {
                hideCrypt(this);
            });
            menu.appendChild(hide);

            var menu2 = new Element('div', {'class': 'menu edit_mode'});
            main.appendChild(menu2);
            if (!editMode)
                menu2.hide();

            var remove = new Element('span', {'hint': 'Remove encryption'}).update('Remove encryption');
            setHintEvent(remove);
            remove.observe('click', function(event) {
                removeCrypt(this);
            });
            menu2.appendChild(remove);
        }

    });
}
function hideCrypt(item) {
    var encrypt = item.up('en-crypt');
    var content = encrypt.down('.encrypted');
    var button = encrypt.down('.cryptButton');
    content.remove();
    button.show();
}
function removeCrypt(item) {
    var encrypt = item.up('en-crypt');
    var content = encrypt.down('.content');
    if (content === null)
        return;

    encrypt.insert({after: new Element('div').update(content.innerHTML)});
    encrypt.remove();
    contentModified = true;
}
function encrypt() {
    jsB.debug("encrypt()");

    var content = jsB.selectedHtml();
    if (content.empty())
        return;

    var data = jsB.encrypt(content);
    if(!data.ok)
        return;

    document.execCommand('insertHTML',false,'<en-crypt hint="' + data.hint + '" id="new_crypt" cipher="RC2" length="64"/>');
    var item = $('new_crypt');
    item.removeAttribute('id');
    loadEnCrypt(item, data.data);
    contentModified = true;
}
function saveLocally(img) {
    var data = jsB.saveImageLocally(img.src, guid);
    if(!data.ok)
        return;

    var res = new Element('en-media', { 'hash' : data.hash, 'type': data.mime });
    img.insert({after: res});
    loadEnMedia(res);
    img.remove();
    contentModified = true;
}
function loadVersion(hash) {
    if (hash in versions)
        setContent(versions[hash]);
}

function nodeToText(node) {
    var text = '';
    var childCount = node.childNodes.length;
    for (var i = 0; i<childCount; i++) {
        var child = node.childNodes[i];
        if (child.nodeType === 3) {
            var txt = child.data.trim();
            text = text + txt;

            if (txt.length > 0)
                text = text + ' ';
        } else if (child.nodeType === 1) {
            var nodeName = child.tagName.toLowerCase();

            if (specialTags.indexOf(nodeName) < 0)
                text = text + nodeToText(child);

            if (breakTags.indexOf(nodeName) > 0)
                text = text + '\n';
        }
    }
    return text;
}

function exportToText() {
    return nodeToText($('body'));
}

function cloneNodesR(oldNode, newNode, img) {
    var childCount = oldNode.childNodes.length;
    for (var i = 0; i<childCount; i++) {
        var child = oldNode.childNodes[i];
        if (child.nodeType === 3) {
            newNode.appendChild(document.createTextNode(child.data));
        } else if (child.nodeType === 1) {
            var nodeName = child.tagName.toLowerCase();
            if (nodeName === 'en-todo')
                nodeName = 'div';

            if (specialTags.indexOf(nodeName) < 0) {
                var el = document.createElement(nodeName);

                var attrCount = child.attributes.length;
                for (var j = 0; j<attrCount; j++) {
                    var attr = child.attributes[j];
                    el.setAttribute(attr.nodeName, attr.value);
                }
                newNode.appendChild(el);

                cloneNodesR(child, el, img);
            } else {
                if (nodeName === 'en-media') {
                    var type = child.readAttribute('type').toLowerCase();
                    if (imageTypes.indexOf(type) >= 0) {
                        var el = document.createElement('img');

                        var attrCount = child.attributes.length;
                        for (var j = 0; j<attrCount; j++) {
                            var attr = child.attributes[j];
                            if ((attr.nodeName != 'hash') && (attr.nodeName != 'type') && (attr.nodeName != 'contenteditable'))
                                el.setAttribute(attr.nodeName, attr.value);
                        }
                        var hash = child.readAttribute('hash');
                        var fileName = jsB.getResourceFileName(hash);
                        if (fileName.length === 0) {
                            fileName = hash;
                            if (type ==='image/jpeg')
                                fileName = fileName + '.jpg';
                            else if (type ==='image/png')
                                fileName = fileName + '.png';
                            else if (type ==='image/gif')
                                fileName = fileName + '.gif';
                        }
                        el.setAttribute('src', fileName);
                        img.add(hash, fileName);

                        newNode.appendChild(el);
                    }
                }
            }
        }

    }
}

function exportToHtml() {
    var html = document.implementation.createDocument("http://www.w3.org/1999/xhtml", "html", null);
    var body = document.createElementNS("http://www.w3.org/1999/xhtml", "body");
    html.documentElement.appendChild(body);
    var content = $('body');

    function ImagesObj()
    {
        this.images = [];
    }
    ImagesObj.prototype.add = function(id, fileName)
    {
        var i = {};
        i['id'] = id;
        i['fileName'] = fileName;
        this.images.push(i);
    }
    var img = new ImagesObj();

    var attrCount = content.attributes.length;
    for (var j = 0; j<attrCount; j++) {
        var attr = content.attributes[j];
        body.setAttribute(attr.nodeName, attr.value);
    }

    cloneNodesR(content, body, img);

    var XMLS = new XMLSerializer();

    var result = {};
    result['content'] = XMLS.serializeToString(html);
    result['images'] = img.images;

    delete html;
    delete XMLS;
    delete img;

    return result;
}

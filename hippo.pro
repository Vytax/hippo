# -------------------------------------------------
# Project created by QtCreator 2011-02-15T21:12:14
# -------------------------------------------------
QT +=   network \
        xml \
        sql

lessThan(QT_MAJOR_VERSION, 5) {
    QT += webkit

    INCLUDEPATH  += /usr/include/poppler/qt4
    LIBS         += -lpoppler-qt4

} else {
    QT += webkitwidgets \
          concurrent

    INCLUDEPATH  += /usr/include/poppler/qt5
    LIBS         += -lpoppler-qt5
}


TARGET = hippo
TEMPLATE = app
include(3rdparty/qtsingleapplication/qtsingleapplication.pri)
include(3rdparty/zlib.pri)
include(3rdparty/rc2.pri)
include(3rdparty/QFreeDesktopMime.pri)
include(3rdparty/qblowfish.pri)
include(3rdparty/spellcheck.pri)
SOURCES += main.cpp \
    tbinaryprotocol.cpp \
    edamprotocol.cpp \
    notebook.cpp \
    note.cpp \
    netmanager.cpp \
    tag.cpp \
    resource.cpp \
    sql.cpp \
    mainwindow.cpp \
    listitem.cpp \
    treeitem.cpp \
    customnetworkaccessmanager.cpp \
    recourcereply.cpp \
    jsbridge.cpp \
    passworddialog.cpp \
    pdfreply.cpp \
    pdfcache.cpp \
    pdfpagedialog.cpp \
    syncget.cpp \
    sync.cpp \
    syncpost.cpp \
    userinfo.cpp \
    urldialog.cpp \
    notebookswidget.cpp \
    tagswidget.cpp \
    noteswidget.cpp \
    insertimage.cpp \
    oauth.cpp \
    dbspellsettings.cpp \
    enml2.cpp \
    customwebpage.cpp \
    taglabel.cpp \
    renamedialog.cpp \
    customwebview.cpp \
    mime.cpp \
    noteinfodialog.cpp \
    optionsdialog.cpp
HEADERS += tbinaryprotocol.h \
    edamprotocol.h \
    notebook.h \
    note.h \
    netmanager.h \
    tag.h \
    resource.h \
    sql.h \
    mainwindow.h \
    listitem.h \
    treeitem.h \
    customnetworkaccessmanager.h \
    recourcereply.h \
    jsbridge.h \
    passworddialog.h \
    pdfreply.h \
    pdfcache.h \
    pdfpagedialog.h \
    syncget.h \
    sync.h \
    syncpost.h \
    userinfo.h \
    urldialog.h \
    notebookswidget.h \
    tagswidget.h \
    noteswidget.h \
    insertimage.h \
    oauth.h \
    dbspellsettings.h \
    enml2.h \
    customwebpage.h \
    taglabel.h \
    renamedialog.h \
    customwebview.h \
    mime.h \
    noteinfodialog.h \
    optionsdialog.h
OBJECTS_DIR = tmp
MOC_DIR = tmp
RCC_DIR = tmp
UI_DIR = tmp

FORMS += \
    mainwindow.ui \
    userinfo.ui \
    insertimage.ui \
    optionsdialog.ui

RESOURCES += \
    res.qrc

OTHER_FILES += \
    html/prototype.js \
    html/editor.js \
    html/style.css \
    html/pdfbrowser.js \
    html/noteajax.html \
    html/mainajax.html \
    TODO.txt













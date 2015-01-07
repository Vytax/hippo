LIBS += -lhunspell
QT += webkit core

SOURCES += $$PWD/spellcheck/spellcheck.cpp \
    $$PWD/spellcheck/speller.cpp \
    $$PWD/spellcheck/qtwebkitplugin.cpp

HEADERS += $$PWD/spellcheck/spellcheck.h \
    $$PWD/spellcheck/speller.h \
    $$PWD/spellcheck/qwebkitplatformplugin.h \
    $$PWD/spellcheck/qtwebkitplugin.h

DEFINES *= QT_STATICPLUGIN

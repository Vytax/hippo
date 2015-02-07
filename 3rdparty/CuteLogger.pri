INCLUDEPATH += $$PWD/Cutelogger/include

SOURCES += $$PWD/Cutelogger/src/Logger.cpp \
           $$PWD/Cutelogger/src/AbstractAppender.cpp \
           $$PWD/Cutelogger/src/AbstractStringAppender.cpp \
           $$PWD/Cutelogger/src/ConsoleAppender.cpp \
           $$PWD/Cutelogger/src/FileAppender.cpp \
           $$PWD/Cutelogger/src/RollingFileAppender.cpp

HEADERS += $$PWD/Cutelogger/include/Logger.h \
           $$PWD/Cutelogger/include/CuteLogger_global.h \
           $$PWD/Cutelogger/include/AbstractAppender.h \
           $$PWD/Cutelogger/include/AbstractStringAppender.h \
           $$PWD/Cutelogger/include/ConsoleAppender.h \
           $$PWD/Cutelogger/include/FileAppender.h \
           $$PWD/Cutelogger/include/RollingFileAppender.h

win32 {
    SOURCES += $$PWD/Cutelogger/src/OutputDebugAppender.cpp
    HEADERS += $$PWD/Cutelogger/include/OutputDebugAppender.h
}

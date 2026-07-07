QT = core network

CONFIG += c++17 cmdline
CONFIG += console

SOURCES += \
        clienthandler.cpp \
        main.cpp \
        server.cpp

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += ../protocol

HEADERS += \
    ../protocol/document.h \
    clienthandler.h \
    server.h \
    ../protocol/protocol.h

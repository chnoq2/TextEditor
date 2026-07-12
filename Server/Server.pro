QT = core network
QT += core gui network widgets
QT += core-private
QT += gui-private


CONFIG += c++17 cmdline
CONFIG += console
CONFIG -= debug_and_release


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
    ../protocol/supporrtive_structures_modules.h \
    clienthandler.h \
    server.h \
    ../protocol/protocol.h

QT = core network

CONFIG += c++17 cmdline

SOURCES += \
        clienthandler.cpp \
        main.cpp \
        server.cpp

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    clienthandler.h \
    server.h

INCLUDEPATH += ../Shared
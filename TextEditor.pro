QT += core gui network widgets

CONFIG += c++17

win32-msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    netclient.cpp

HEADERS += \
    mainwindow.h \
    netclient.h \
    protocol/document.h \
    protocol/protocol.h

FORMS += \
    mainwindow.ui

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DISTFILES += \
    .dockerignore \
    .gitignore \
    Dockerfile

RESOURCES += \
    text_editor_resources.qrc


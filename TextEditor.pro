QT += widgets
QT += core gui
QT += core gui network
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
    netclient.h

FORMS += \
    mainwindow.ui

INCLUDEPATH += Shared

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

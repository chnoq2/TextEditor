QT += core gui network widgets
QT += core-private
QT += gui-private



CONFIG += c++17
CONFIG -= debug_and_release

win32-msvc {
    QMAKE_CXXFLAGS += /utf-8
}

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    netclient.cpp \
    settingsdialog.cpp

HEADERS += \
    mainwindow.h \
    netclient.h \
    protocol/supporrtive_structures_modules.h \
    settingsdialog.h \
    protocol/document.h \
    protocol/protocol.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui

qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


DISTFILES += \
    .dockerignore \
    .gitignore \
    Dockerfile

RESOURCES += \
    text_editor_resources.qrc


CONFIG(debug, debug|release) {
    DESTDIR      = ./build/debug
    OBJECTS_DIR  = ./build/debug/.obj
    MOC_DIR      = ./build/debug/.moc
    RCC_DIR      = ./build/debug/.rcc
    UI_DIR       = ./build/debug/.ui
} else {
    DESTDIR      = ./build/release
    OBJECTS_DIR  = ./build/release/.obj
    MOC_DIR      = ./build/release/.moc
    RCC_DIR      = ./build/release/.rcc
    UI_DIR       = ./build/release/.ui
}

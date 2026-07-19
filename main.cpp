#include "mainwindow.h"
#include <QCoreApplication>
#include <QApplication>
#include <QTimer>
#include <QDebug>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("TextEditorApp");
    QCoreApplication::setApplicationName("TextEditor");

    MainWindow w;
    w.show();
    return QApplication::exec();
}

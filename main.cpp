#include "mainwindow.h"
#include <QCoreApplication>
#include <QApplication>
#include <QTimer>
#include <QDebug>
#include <windows.h>
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return QApplication::exec();
}

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "netclient.h"
#include <QTextCursor>

#include <QIcon>
#include <QCoreApplication>
#include <QDir>

MainWindow::MainWindow(QWidget *parent): QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_client = new NetClient(this);

    this->setWindowIcon(QIcon(":/source/icon_app.png"));


}

MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::on_pushButton_clicked()
{
    QString ip = ui->IP_line->text();

    quint16 port = ui->host_line->text().toUShort();

    m_client->connectToServer(ip,port);
}


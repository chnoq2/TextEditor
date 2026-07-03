#include "netclient.h"

NetClient::NetClient(QObject *parent) : QObject(parent), m_socket(new QTcpSocket(this)), m_nextBlockSize(0){
    connect(&m_socket, &QTcpSocket::connected, this, &NetClient::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &NetClient::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this , &NetClient::onReadyStatus);
}

NetClient::~NetClient(){

}

void NetClient::connectToServer(const QString &host, quint16 port){
    qDebug() << "try to connect" << host << ":" << port;
    m_socket.connectToHost(host,port);
}


void NetClient::sendInsert(int position, const QString &text){
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << position << text;

    sendPacket(Protocol::TextInsert, buffer);

}

void NetClient::sendDelete(int position, int length){
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);


    out << position << length;

    sendPacket(Protocol::TextInsert, buffer);

}

void NetClient::sendCursorMove(int position){
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);


    out << position;

    sendPacket(Protocol::CursorMove, buffer);
}


void NetClient::sendPacket(quint8 msgType, const QByteArray &payload){
    if (m_socket.state() != QAbstractSocket::ConnectedState) return;

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);


    out << quint32(0) << msgType;

    packet.append(payload);

    out.device()->seek(0);
    out << quint32(packet.size() - sizeof(quint32));

    m_socket.write(packet);
}


void NetClient::onConnected(){
    m_nextBlockSize = 0;
    qDebug() << "connected !";
}

void NetClient::onDisconnected(){
    m_nextBlockSize = 0;
    qDebug() << "Disconnected";
}

void NetClient::onReadyStatus(){

}

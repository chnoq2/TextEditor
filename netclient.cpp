#include "netclient.h"
#include "protocol/document.h"

NetClient::NetClient(QObject *parent) : QObject(parent), m_socket(this), m_nextBlockSize(0){

    connect(&m_socket, &QTcpSocket::connected, this, &NetClient::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &NetClient::onDisconnected);
    connect(&m_socket, &QTcpSocket::readyRead, this , &NetClient::onReadyStatus);
}

NetClient::~NetClient()
{

}

void NetClient::connectToServer(const QString &host, quint16 port){
    qDebug() << "try to connect" << host << ":" << port;
    m_socket.connectToHost(host,port);
}


void NetClient::sendInsert(int position, const QString &text)
{
    qDebug() << "[SENDING] sendInsert called. Position:" << position << "Text:" << text;  // пока что добабавим для логов


    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);

    // из за обновлеленного протокола, добавляем новые параметрры для функции
    // где первые {} это изображения в виде массива и  вторые {} использованные инструменты

    Protocol::TextInsertData data(position, text,{},{});
    out << data;

    sendPacket(Protocol::TextInsert, buffer);

}

void NetClient::sendDelete(int position, int length){

    qDebug() << "[SENDING] sendDelete called. Position:" << position << "Length:" << length; // пока что добабавим для логов

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);

    // из за обновлеленного протокола, добавляем новые параметрры для функции
    // где первые {} это изображения в виде массива и  вторые {} использованные инструменты

    Protocol::TextDeleteData data(position, length,{},{});
    out << data;

    sendPacket(Protocol::TextDelete, buffer);

}

void NetClient::sendCursorMove(int position){
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);


    out << position;

    sendPacket(Protocol::CursorMove, buffer);
}


void NetClient::sendPacket(quint8 msgType, const QByteArray &payload){
    if (m_socket.state() != QAbstractSocket::ConnectedState) {
        qDebug() << "[ERROR] packet hasn`t sent, socket has state:" << m_socket.state();  // пока что добабавим для логов

        return;
    }

    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);


    out << quint32(0) << msgType;

    packet.append(payload);

    out.device()->seek(0);
    out << quint32(packet.size() - sizeof(quint32));

    m_socket.write(packet);
    m_socket.flush();



}


void NetClient::onConnected(){
    m_nextBlockSize = 0;
    qDebug() << "connected !";
    emit connected();
}

void NetClient::onDisconnected(){
    m_nextBlockSize = 0;
    qDebug() << "Disconnected";
}

void NetClient::onReadyStatus()
{
    QDataStream in(&m_socket);

    while (true) {
        if (m_nextBlockSize == 0) {
            if (m_socket.bytesAvailable() < (qint64)sizeof(quint32)) break;
            in >> m_nextBlockSize;
        }

        if (m_socket.bytesAvailable() < (qint64)m_nextBlockSize) break;

        quint8 msgType;
        in >> msgType;

        QByteArray payload = m_socket.read(m_nextBlockSize - sizeof(quint8));
        m_nextBlockSize = 0;

        handlePacket(msgType, payload);
    }
}

void NetClient::handlePacket(quint8 type, const QByteArray &payload)
{
    QDataStream in(payload);

    switch (type) {
    case Protocol::DockSnapShot: {
        document_standard doc;
        in >> doc;
        emit documentSnapshotReceived(doc.get_text());
        qDebug() << "[RECV] Snapshot received, length:" << doc.get_length();
        break;
    }
    case Protocol::TextInsert: {
        Protocol::TextInsertData data;
        in >> data;
        emit textInserted(data.get_position(), data.get_text());
        qDebug() << "[RECV] Insert at" << data.get_position() << ":" << data.get_text();
        break;
    }
    case Protocol::TextDelete: {
        Protocol::TextDeleteData data;
        in >> data;
        emit textDeleted(data.get_position(), data.get_length());
        qDebug() << "[RECV] Delete at" << data.get_position() << "len:" << data.get_length();
        break;
    }
    case Protocol::CursorMove: {
        int position;
        in >> position;
        emit cursorMoved(position);
        break;
    }
    case Protocol::AssignRole: {
        quint8 role; int id;
        in >> role >> id;
        m_role = static_cast<Protocol::UserRole>(role);
        m_myId = id;
        emit roleAssigned(m_role);
        break;
    }
    case Protocol::UserList: {
        Protocol::UserListData data;
        in >> data;
        emit userListUpdated(data.get_users());
        break;
    }
    case Protocol::TypingStart: {
        int userId; in >> userId;
        emit typingStarted(userId);
        break;
    }
    case Protocol::TypingStop: {
        int userId; in >> userId;
        emit typingStopped(userId);
        break;
    }
    default:
        qDebug() << "[RECV] Unknown packet type:" << type;
        break;
    }
}

void NetClient::sendSetName(const QString &name)
{
    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);
    out << name;
    sendPacket(Protocol::SetName, buffer);
}

void NetClient::sendTypingStart()
{
    sendPacket(Protocol::TypingStart, QByteArray());
}

void NetClient::sendTypingStop()
{
    sendPacket(Protocol::TypingStop, QByteArray());
}
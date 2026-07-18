#include "netclient.h"
#include "protocol/document.h"

NetClient::NetClient(QObject *parent) : QObject(parent), m_socket(this), m_nextBlockSize(0){

    connect(&m_socket, &QTcpSocket::connected, this, &NetClient::onConnected);
    connect(&m_socket, &QTcpSocket::disconnected, this, &NetClient::onDisconnected);
    connect(&m_socket, &QTcpSocket::errorOccurred, this, &NetClient::onSocketError);
    connect(&m_socket, &QTcpSocket::readyRead, this , &NetClient::onReadyStatus);
}

NetClient::~NetClient(){
}

void NetClient::connectToServer(const QString &host, quint16 port){
    qDebug() << "try to connect" << host << ":" << port;
    m_socket.connectToHost(host,port);
}

void NetClient::disconnectFromServer(){
    m_socket.disconnectFromHost();
}

void NetClient::sendInsert(int paragraphIdx,int position_in_paragraph, const QString &text,
const QList<ImageElement>& images, const QList<TextStyleElement>& styles)
{
    qDebug() << "[SENDING] sendInsert called. paragraph :" << paragraphIdx
             << "position in paragraph :" << position_in_paragraph
             << "text :" << text;

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);

    Protocol::TextInsertData data(paragraphIdx, position_in_paragraph,text,images,styles);
    out << data;

    sendPacket(Protocol::TextInsert, buffer);

}

void NetClient::sendDelete(int paragraphIdx, int posInParagraph, int length)
{
    qDebug() << "[SENDING] sendDelete called. Paragraph:" << paragraphIdx
             << "Position in paragraph:" << posInParagraph << "Length:" << length;

    QByteArray buffer;
    QDataStream out(&buffer, QIODevice::WriteOnly);

    Protocol::TextDeleteData data(paragraphIdx, posInParagraph, length);
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
    emit disconnected();
}

void NetClient::onSocketError(QAbstractSocket::SocketError error){
    Q_UNUSED(error);
    qDebug() << "[ERROR] socket error:" << m_socket.errorString();
    emit connectionError(m_socket.errorString());
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
        emit documentSnapshotReceived(payload);
        break;
    }
    case Protocol::TextInsert: {
        Protocol::TextInsertData data;
        in >> data;
        emit textInserted(data.get_paragraph_index(),data.get_position_in_paragraph(),data.get_text(),
                          data.get_images(), data.get_styles());
        qDebug() << "[RECV] Insert at Paragraph:" << data.get_paragraph_index()
                 << "Pos:" << data.get_position_in_paragraph() << ":" << data.get_text();
        break;
    }
    case Protocol::TextDelete: {
        Protocol::TextDeleteData data;
        in >> data;
   emit textDeleted(data.get_paragraph_index(), data.get_position_in_paragraph(), data.get_length());
        qDebug() << "[RECV] Delete at Paragraph:" << data.get_paragraph_index()
                 << "Pos:" << data.get_position_in_paragraph() << "len:" << data.get_length();
        break;
    }
    case Protocol::CursorMove: {
        int position;
        in >> position;
        emit cursorMoved(position);
        break;
    }
    case Protocol::AssignRole: {
        // Роли (Writer/Reader) отключены
        quint8 role; int id;
        in >> role >> id;
        Q_UNUSED(role);
        m_myId = id;
        // m_role = static_cast<Protocol::UserRole>(role);
        // emit roleAssigned(m_role);
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

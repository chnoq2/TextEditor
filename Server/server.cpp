#include "server.h"
#include "protocol.h"
#include <QDataStream>
#include <QDebug>
#include <QDateTime>


Server::Server(QObject *parent) : QObject(parent) {}

bool Server::start(quint16 port)
{
    connect(&m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!m_server.listen(QHostAddress::Any, port)) {
        qDebug() << "[Error] Failed to start server:" << m_server.errorString();
        return false;
    }
    qDebug() << "Server started on port" << port;
    return true;
}

void Server::onNewConnection()
{
    QTcpSocket *socket = m_server.nextPendingConnection();
    ClientHandler *client = new ClientHandler(socket, this);

    connect(client, &ClientHandler::packetReceived, this, &Server::onPacketReceived);
    connect(client, &ClientHandler::disconnected, this, &Server::onClientDisconnected);

    m_clients.append(client);

    Protocol::UserRole role = (m_clients.size() == 1) ? Protocol::Writer : Protocol::Reader;

    qDebug() << "New client connected. ID:" << client->id()
             << "IP:" << socket->peerAddress().toString()
             << "Assigned Role:" << (role == Protocol::Writer ? "Writer" : "Reader");

    QByteArray rolePayload;
    QDataStream roleOut(&rolePayload, QIODevice::WriteOnly);
    roleOut << static_cast<quint8>(role);
    client->sendPacket(Protocol::AssignRole, rolePayload);

    QByteArray docPayload;
    QDataStream docOut(&docPayload, QIODevice::WriteOnly);
    docOut << m_document;
    client->sendPacket(Protocol::DockSnapShot, docPayload);

    qDebug() << "Sent DockSnapShot to client ID:" << client->id() << "Current doc length:" << m_document.get_length();


}

void Server::onPacketReceived(ClientHandler *sender, quint8 msgType, QByteArray payload)
{
    QDataStream in(&payload, QIODevice::ReadOnly);

    if (msgType == Protocol::TextInsert)
    {
        Protocol::TextInsertData data;
        in >> data;

        m_document.applyinsert(data);

        qDebug() << "Insert from client ID:" << sender->id()
                 << "at pos:" << data.get_position()
                 << "text:" << data.get_text()
                 << "[Images:" << data.get_images_count() << ", Tools:" << data.get_tools_count()<< "]";

        broadcast(sender, msgType, payload);

    }
    else if (msgType == Protocol::TextDelete)
    {
        Protocol::TextDeleteData data;
        in >> data;
        m_document.applyDelete(data);

        qDebug() << "Delete from client ID:" << sender->id()
                 << "at pos:" << data.get_position()
                 << "len:" << data.get_length();

        broadcast(sender, msgType, payload);

    }
    else if (msgType == Protocol::CursorMove)
    {
        qDebug() << "Cursor move event from client ID:" << sender->id();
        broadcast(sender, msgType, payload);
    }
}

void Server::onClientDisconnected(ClientHandler *sender)
{
    m_clients.removeAll(sender);
    sender->deleteLater();
}

void Server::broadcast(ClientHandler *except, quint8 msgType, const QByteArray &payload)
{
    for (ClientHandler *client : m_clients) {
        if (client != except) {
            client->sendPacket(msgType, payload);
        }
    }
}

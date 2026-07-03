#include "server.h"
#include "Protocol.h"
#include <QDataStream>

Server::Server(QObject *parent) : QObject(parent) {}

bool Server::start(quint16 port)
{
    connect(&m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!m_server.listen(QHostAddress::Any, port)) {
        qDebug() << "Failed to start server:" << m_server.errorString();
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

    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << m_document;
    client->sendPacket(Protocol::DockSnapShot, payload);
}

void Server::onPacketReceived(ClientHandler *sender, quint8 msgType, QByteArray payload)
{
    QDataStream in(&payload, QIODevice::ReadOnly);

    if (msgType == Protocol::TextInsert) {
        int position; QString text;
        in >> position >> text;

        m_document.insert(position, text);
        qDebug() << "Insert at" << position << ":" << text;

        broadcast(sender, msgType, payload);

    } else if (msgType == Protocol::TextDelete) {
        int position, length;
        in >> position >> length;

        m_document.remove(position, length);
        qDebug() << "Delete at" << position << "len" << length;

        broadcast(sender, msgType, payload);

    } else if (msgType == Protocol::CursorMove) {
        // Позицию курсора не храним, просто ретранслируем
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
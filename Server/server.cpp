#include "server.h"
#include "protocol.h"
#include <QDataStream>
#include <QDebug>
#include <QDateTime>
#include <QPointer>


Server::Server(QObject *parent) : QObject(parent) {}

bool Server::start(quint16 port)
{
    connect(&m_server, &QTcpServer::newConnection, this, &Server::onNewConnection);

    if (!m_server.listen(QHostAddress::Any, port)) {
        qWarning() << "[Error] Failed to start server:" << m_server.errorString();
        return false;
    }
    qDebug() << "Server started on port" << port;
    return true;
}

void Server::onNewConnection()
{
    QPointer<QTcpSocket> socket = m_server.nextPendingConnection();
    QPointer<ClientHandler> client = new ClientHandler(socket, this);

    connect(client, &ClientHandler::packetReceived, this, &Server::onPacketReceived);
    connect(client, &ClientHandler::disconnected, this, &Server::onClientDisconnected);

    m_clients.append(client);

    // Protocol::UserRole role = (m_clients.size() == 1) ? Protocol::Writer : Protocol::Reader;
    // client->setRole(role);
    client->setName(QString("User%1").arg(client->id()));
    qDebug() << "New client connected. ID:" << client->id()
             << "IP:" << socket->peerAddress().toString();

    // AssignRole теперь используется только чтобы сообщить клиенту его id
    QByteArray rolePayload;
    QDataStream roleOut(&rolePayload, QIODevice::WriteOnly);
    roleOut << static_cast<quint8>(0) << client->id();
    client->sendPacket(Protocol::AssignRole, rolePayload);


    broadcastUserList();
}

void Server::onPacketReceived(ClientHandler *sender, quint8 msgType, QByteArray payload)
{
    QDataStream in(&payload, QIODevice::ReadOnly);

    switch (static_cast<Protocol::MessageType>(msgType))
    {
    case Protocol::TextInsert:
    {
        Protocol::TextInsertData data;
        in >> data;
        if (in.status() != QDataStream::Ok) {
            qWarning() << "Malformed TextInsert packet from client ID:" << sender->id();
            return;
        }

        m_document.applyinsert(data);

        qDebug() << "Insert from client ID:" << sender->id()
                 << "at paragraph:" << data.get_paragraph_index()
                 << "position in paragraph:" << data.get_position_in_paragraph()
                << "[Images:" << data.get_images_count() << ", Tools:" << data.get_tools_count() << "]";

        broadcast(sender, msgType, payload);
        break;
    }
    case Protocol::TextDelete:
    {
        Protocol::TextDeleteData data;
        in >> data;
        if (in.status() != QDataStream::Ok) {
            qWarning() << "Malformed TextDelete packet from client ID:" << sender->id();
            return;
        }
        m_document.applyDelete(data);

        qDebug() << "Delete from client ID:" << sender->id()
                 << "at paragraph:" << data.get_paragraph_index()
                 << "pos in paragraph:" << data.get_position_in_paragraph()
                 << "len:" << data.get_length();

        broadcast(sender, msgType, payload);
        break;
    }
    case Protocol::TextRestyle:
    {
        Protocol::TextRestyleData data;
        in >> data;
        if (in.status() != QDataStream::Ok) {
            qWarning() << "Malformed TextRestyle packet from client ID:" << sender->id();
            return;
        }

        m_document.applyRestyle(data);

        qDebug() << "Restyle from client ID:" << sender->id()
                 << "at paragraph:" << data.get_index();

        broadcast(sender, msgType, payload);
        break;
    }
    case Protocol::CursorMove:
    {
        qDebug() << "Cursor move event from client ID:" << sender->id();
        broadcast(sender, msgType, payload);
        break;
    }
    case Protocol::SetName:
    {
        QString name;
        in >> name;
        if (in.status() != QDataStream::Ok) {
            qWarning() << "Malformed SetName packet from client ID:" << sender->id();
            return;
        }
        sender->setName(name);
        broadcastUserList();

        if (m_hasDocument)
        {
            QByteArray docPayload;
            QDataStream docOut(&docPayload, QIODevice::WriteOnly);
            docOut << m_document;

            sender->sendPacket(Protocol::DockSnapShot, docPayload);
            qDebug() << "Sent current document to newly authorized client ID:" << sender->id();
        }
        break;
    }
    case Protocol::TypingStart:
    case Protocol::TypingStop:
    {
        broadcastFrom(sender, static_cast<Protocol::MessageType>(msgType));
        break;
    }
    case Protocol::DockSnapShot:
    {
        in >> m_document;
        if (in.status() != QDataStream::Ok) {
            qWarning() << "Malformed DockSnapShot packet from client ID:" << sender->id();
            return;
        }
        m_hasDocument = true;

        qDebug() << "Received new document snapshot from client ID:" << sender->id() << "Length:" << m_document.get_length();
        QByteArray forwardPayload;
        QDataStream out(&forwardPayload, QIODevice::WriteOnly);
        out << m_document;

        broadcast(sender, Protocol::DockSnapShot, forwardPayload);
        break;
    }
    default:
        break;
    }
}

void Server::onClientDisconnected(ClientHandler *sender)
{
    m_clients.removeAll(sender);
    sender->deleteLater();
    broadcastUserList();
}

void Server::broadcastUserList()
{
    QList<Protocol::ClientInfo> infos;
    for (const QPointer<ClientHandler> &c : m_clients)
        infos.append(Protocol::ClientInfo(c->id(), c->name(), c->role()));

    Protocol::UserListData data(infos);
    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << data;

    for (const QPointer<ClientHandler> &c : m_clients)
        c->sendPacket(Protocol::UserList, payload);
}

void Server::broadcastFrom(ClientHandler *sender, Protocol::MessageType msgType)
{
    QByteArray payload;
    QDataStream out(&payload, QIODevice::WriteOnly);
    out << sender->id();

    for (const QPointer<ClientHandler> &c : m_clients) {
        if (c != sender)
            c->sendPacket(msgType, payload);
    }
}

void Server::broadcast(ClientHandler *except, quint8 msgType, const QByteArray &payload)
{
    for (const QPointer<ClientHandler> &client : m_clients) {
        if (client != except) {
            client->sendPacket(msgType, payload);
        }
    }
}


#include "clienthandler.h"
#include <QDataStream>

int ClientHandler::s_nextId = 1;

ClientHandler::ClientHandler(QTcpSocket *socket, QObject *parent)
    : QObject(parent), m_socket(socket), m_nextBlockSize(0)
{
    m_id = s_nextId++;
    m_socket->setParent(this);

    connect(m_socket, &QTcpSocket::readyRead, this, &ClientHandler::onReadyRead);
    connect(m_socket, &QTcpSocket::disconnected, this, &ClientHandler::onDisconnected);

    qDebug() << "Client connected, id=" << m_id;
}

void ClientHandler::sendPacket(quint8 msgType, const QByteArray &payload)
{
    QByteArray packet;
    QDataStream out(&packet, QIODevice::WriteOnly);
    out << quint32(0) << msgType;
    packet.append(payload);
    out.device()->seek(0);
    out << quint32(packet.size() - sizeof(quint32));
    m_socket->write(packet);
}

void ClientHandler::onReadyRead()
{
    QDataStream in(m_socket);

    while (true)
    {
        in.startTransaction();

        quint32 blockSize;
        quint8 msgType;

        in >> blockSize >> msgType;

        int payloadSize = blockSize - sizeof(quint8);
        QByteArray payload;
        payload.resize(payloadSize);
        in.readRawData(payload.data(), payloadSize);

        if (!in.commitTransaction()) {
            break;
        }

        emit packetReceived(this, msgType, payload);
    }
}

void ClientHandler::onDisconnected()
{
    qDebug() << "Client disconnected, id=" << m_id;
    emit disconnected(this);
}

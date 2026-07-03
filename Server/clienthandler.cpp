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

    while (true) {
        if (m_nextBlockSize == 0) {
            // ждём заголовок (4 байта длины)
            if (m_socket->bytesAvailable() < sizeof(quint32)) break;
            in >> m_nextBlockSize;
        }

        // ждём весь пакет
        if (m_socket->bytesAvailable() < m_nextBlockSize) break;

        quint8 msgType;
        in >> msgType;

        // payload = всё что после типа
        QByteArray payload = m_socket->read(m_nextBlockSize - sizeof(quint8));
        m_nextBlockSize = 0;

        emit packetReceived(this, msgType, payload);
    }
}

void ClientHandler::onDisconnected()
{
    qDebug() << "Client disconnected, id=" << m_id;
    emit disconnected(this);
}
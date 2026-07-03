#ifndef CLIENTHANDLER_H
#define CLIENTHANDLER_H

#include <QObject>
#include <QTcpSocket>
#include "protocol.h"

class ClientHandler : public QObject
{
    Q_OBJECT
public:
    explicit ClientHandler(QTcpSocket *socket, QObject *parent = nullptr);

    void sendPacket(quint8 msgType, const QByteArray &payload);
    int id() const { return m_id; }

signals:
    void packetReceived(ClientHandler *sender, quint8 msgType, QByteArray payload);
    void disconnected(ClientHandler *sender);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    QTcpSocket *m_socket;
    quint32 m_nextBlockSize;
    int m_id;
    static int s_nextId;
};

#endif
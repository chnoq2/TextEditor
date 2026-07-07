#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <QTcpServer>
#include <QList>
#include "clienthandler.h"
#include "document.h"

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    bool start(quint16 port);

private slots:
    void onNewConnection();
    void onPacketReceived(ClientHandler *sender, quint8 msgType, QByteArray payload);
    void onClientDisconnected(ClientHandler *sender);

private:
    void broadcast(ClientHandler *except, quint8 msgType, const QByteArray &payload);

    QTcpServer m_server;
    QList<ClientHandler*> m_clients;
    document_standard m_document;                                   // было QString m_document;
};

#endif

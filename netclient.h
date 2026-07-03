#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include "protocol.h"
#include <QDataStream>

class NetClient: public QObject
{
    Q_OBJECT

    private:
    QTcpSocket m_socket;
    Protocol::MessageType msgType;
    Protocol::UserRole m_role;
    quint32 m_nextBlockSize;

    public:

    explicit NetClient(QObject *parent = nullptr);
    ~NetClient();


    void sendInsert(int position, const QString &text);
    void sendDelete(int position, int length);
    void sendCursorMove(int position);

    void connectToServer(const QString &host, quint16 port);
    void sendPacket(quint8 msgType, const QByteArray &payload);

    private slots:

    void onConnected();
    void onDisconnected();
    void onReadyStatus();

    signals:

    void textInserted(int position, const QString &text);
    void textDeleted(int position, int length);
    void documentSnapshotReceived(const QString &text);


};

#endif

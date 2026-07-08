#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include "protocol/protocol.h"
#include "protocol/protocol.h"
#include <QDataStream>
#include <QDebug>
#include <QDebug>

class NetClient: public QObject
{
    Q_OBJECT

    private:
    QTcpSocket m_socket;
    Protocol::MessageType msgType;
    Protocol::UserRole m_role;
    quint32 m_nextBlockSize;
    void handlePacket(quint8 type, const QByteArray &payload);
    int m_myId = -1;

    public:

    explicit NetClient(QObject *parent = nullptr);
    ~NetClient();


    void sendInsert(int position, const QString &text);
    void sendDelete(int position, int length);
    void sendCursorMove(int position);

    void connectToServer(const QString &host, quint16 port);
    void sendPacket(quint8 msgType, const QByteArray &payload);

    void sendSetName(const QString &name);
    void sendTypingStart();
    void sendTypingStop();
    int myId() const { return m_myId; }

    Protocol::UserRole role() const {return m_role;}

    private slots:

    void onConnected();
    void onDisconnected();
    void onReadyStatus();

    signals:

    void textInserted(int position, const QString &text);
    void textDeleted(int position, int length);
    void documentSnapshotReceived(const QString &text);

    void packetReceived(quint8 msgType, const QByteArray &payload);
    void roleAssigned(Protocol::UserRole role);
    void cursorMoved(int position);

    void connected();


    void userListUpdated(const QList<Protocol::ClientInfo> &users);
    void typingStarted(int userId);
    void typingStopped(int userId);

};

#endif

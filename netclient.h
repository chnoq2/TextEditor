#ifndef NETCLIENT_H
#define NETCLIENT_H

#include <QObject>
#include <QString>
#include <QTcpSocket>
#include "protocol/protocol.h"
#include <QDataStream>
#include <QDebug>

class NetClient: public QObject
{
    Q_OBJECT

private:
    QTcpSocket m_socket;
    Protocol::MessageType msgType;
    // Protocol::UserRole m_role;
    quint32 m_nextBlockSize = 0;
    void handlePacket(quint8 type, const QByteArray &payload);
    int m_myId = -1;

public:

    explicit NetClient(QObject *parent = nullptr);
    ~NetClient();


    void sendInsert(int paragraphIdx,int position_in_paragraph, const QString &text,
                    const QList<ImageElement>& images = {}, const QList<TextStyleElement>& styles = {});

    void sendDelete(int paragraphIdx, int position_in_paragraph, int length);
    void sendRestyle(int paragraphIdx, const TextStyleElement &style); // изменить формат/выравнивание уже существующего диапазона
    void sendCursorMove(int position);

    void connectToServer(const QString &host, quint16 port);
    void disconnectFromServer();
    void sendPacket(quint8 msgType, const QByteArray &payload);

    void sendSetName(const QString &name);
    void sendTypingStart();
    void sendTypingStop();
    int myId() const { return m_myId; }

    // Protocol::UserRole role() const {return m_role;}

private slots:

    void onConnected();
    void onDisconnected();
    void onSocketError(QAbstractSocket::SocketError error);
    void onReadyStatus();

signals:

    void textInserted(int paragraphIdx,int position_in_paragraph, const QString &text,
                      const QList<ImageElement>& images, const QList<TextStyleElement>& styles);
    void textDeleted(int paragraphIdx,int position_in_paragraph, int length);
    void textRestyled(int paragraphIdx, TextStyleElement style);

    void documentSnapshotReceived(const QByteArray &snapshotData);
    void packetReceived(quint8 msgType, const QByteArray &payload);
    // void roleAssigned(Protocol::UserRole role);
    void cursorMoved(int position);

    void connected();
    void disconnected();
    void connectionError(const QString &message);


    void userListUpdated(const QList<Protocol::ClientInfo> &users);
    void typingStarted(int userId);
    void typingStopped(int userId);



};

#endif

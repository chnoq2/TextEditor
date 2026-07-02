#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>

namespace Protocol
{
enum MessageType : quint8 {
    AssignRole = 1,
    DockRequest = 2,
    DockSnapShot = 3,

    TextInsert = 4,
    TextDelete = 5,

    CursorMove = 6
    };

    enum UserRole : quint8 {
        Reader = 0,
        Writer = 1
        };
}

#endif // PROTOCOL_H

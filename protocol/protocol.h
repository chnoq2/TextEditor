#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QtGlobal>
#include <QString>
#include <QList>
#include <QByteArray>
#include <QDataStream>

#include "structures_and_other_elements.h"

namespace Protocol
{
enum MessageType : quint8 {
    AssignRole   = 1,
    DockRequest  = 2,
    DockSnapShot = 3,
    TextInsert   = 4,
    TextDelete   = 5,
    CursorMove   = 6,
    SetName      = 7,   // клиент -> сервер: желаемое имя
    UserList     = 8,   // сервер -> клиенты: полный список участников
    TypingStart  = 9,   // клиент -> сервер -> остальные
    TypingStop   = 10
};

enum UserRole : quint8 {
    Reader = 0,
    Writer = 1
};

enum UserStatus : quint8 {
    LeftStatus   = 0,
    OnlineStatus = 1
};




class TextInsertData
{

private:
    int m_paragraph_index = 0;
    int m_position_in_paragraph = 0;
    QString m_text;
    QList<ImageElement> m_images;    // Хранение бинарных изображений в бинарном виде
    QList<TextStyleElement> m_styles;    // Список примененных инструментов
public:
    TextInsertData() = default;

    TextInsertData(int paragraphIdx, int posInParagraph, const QString& text,
                   const QList<ImageElement>& images, const QList<TextStyleElement>& styles)
        : m_paragraph_index(paragraphIdx), m_position_in_paragraph(posInParagraph),
        m_text(text), m_images(images), m_styles(styles) {}


    int get_paragraph_index() const { return m_paragraph_index; }
    int get_position_in_paragraph() const { return m_position_in_paragraph; }
    QString get_text() const { return m_text; }
    QList<ImageElement> get_images() const { return m_images; }
    QList<TextStyleElement> get_styles() const { return m_styles; }

    int get_textLength() const { return m_text.length(); }
    int get_images_count() const { return m_images.size(); }
    int get_tools_count() const { return m_styles.size(); }


    friend QDataStream &operator<<(QDataStream &out, const TextInsertData &data) {
        out << data.m_paragraph_index << data.m_position_in_paragraph
            << data.m_text << data.m_images << data.m_styles;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TextInsertData &data) {
        in >> data.m_paragraph_index >> data.m_position_in_paragraph
            >> data.m_text >> data.m_images >> data.m_styles;
        return in;
    }


};


class TextDeleteData
{

private:
    int m_paragraph_index = 0;
    int m_position_in_paragraph = 0;
    int m_length = 0;
public:

    TextDeleteData() = default;
    TextDeleteData(int paragraphIdx, int posInParagraph, int length)
        : m_paragraph_index(paragraphIdx), m_position_in_paragraph(posInParagraph), m_length(length) {}

    int get_paragraph_index() const { return m_paragraph_index; }
    int get_position_in_paragraph() const { return m_position_in_paragraph; }
    int get_length() const { return m_length;}


    friend QDataStream &operator<<(QDataStream &out, const TextDeleteData &data) {
            out << data.m_paragraph_index << data.m_position_in_paragraph << data.m_length;
            return out;
        }
    friend QDataStream &operator>>(QDataStream &in, TextDeleteData &data) {
            in >> data.m_paragraph_index >> data.m_position_in_paragraph >> data.m_length;
            return in;
        }

};

class ClientInfo
{
private:
    int m_id = 0;
    QString m_name;
    UserRole m_role = Reader;

public:
    ClientInfo() = default;
    ClientInfo(int id, const QString &name, UserRole role)
        : m_id(id), m_name(name), m_role(role) {}

    int get_id() const { return m_id; }
    QString get_name() const { return m_name; }
    UserRole get_role() const { return m_role; }

    friend QDataStream &operator<<(QDataStream &out, const ClientInfo &c) {
        out << c.m_id << c.m_name << static_cast<quint8>(c.m_role);
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, ClientInfo &c) {
        quint8 role;
        in >> c.m_id >> c.m_name >> role;
        c.m_role = static_cast<UserRole>(role);
        return in;
    }
};

class UserListData
{
private:
    QList<ClientInfo> m_users;
public:
    UserListData() = default;
    explicit UserListData(const QList<ClientInfo> &users) : m_users(users) {}
    QList<ClientInfo> get_users() const { return m_users; }

    friend QDataStream &operator<<(QDataStream &out, const UserListData &d) {
        out << d.m_users;
        return out;
    }
    friend QDataStream &operator>>(QDataStream &in, UserListData &d) {
        in >> d.m_users;
        return in;
    }
};


}; // end namespace

#endif // PROTOCOL_H

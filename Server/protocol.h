#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QDataStream>
#include <QByteArray>
#include <QString>
#include <QList>

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

    enum UserStatus : quint8
    {
        online_staus= 1,
        left_status  = 0
    };


class TextInsertData
{
    private:
    int  m_position =0;
    QString m_text;
    QList<QByteArray> m_images; // хранение  изображений в бинарном виде
    QList<QString> m_usedTools; // примененные инструменты в виде списка

    public:

    TextInsertData() = default;

    TextInsertData(int pos, const QString& text, const QList<QByteArray>& images , const QList<QString> usedTools)
        : m_position(pos), m_text(text), m_images(images), m_usedTools(usedTools) {}

    // геттеры
    int get_position() const {return m_position;}
    QString get_text() const {return m_text;}
    QList<QByteArray> get_images() const {return m_images;}
    QList<QString> get_tools() const {return m_usedTools;}


    // Метаданные для логирования
    int get_text_length() const {return m_text.length();}
    int get_images_count() const {return m_images.size();}
    int get_tools_count() const {return m_usedTools.size();}

    friend QDataStream &operator<<(QDataStream &out, const TextInsertData &data)
    {
        out << data.m_position << data.m_text << data.m_images << data.m_usedTools;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TextInsertData &data)
    {
        in >> data.m_position >> data.m_text >> data.m_images >> data.m_usedTools;
        return in;
    }


};

class TextDeleteData
{
    private:
    int m_position = 0;
    int m_length = 0;
    QList<QByteArray> m_images; // хранение  изображений в бинарном виде
    QList<QString> m_usedTools;  // примененные инструменты в виде списка

    public:

    TextDeleteData() = default;

    TextDeleteData(int position, int length, QList<QByteArray> images, QList<QString> usedTools)
        : m_position(position), m_length(length), m_images(images), m_usedTools(usedTools) {}

    // геттеры

    int get_position() const {return m_position;}
    int get_length() const {return m_length;}
    QList<QByteArray> get_images() const {return m_images;}
    QList<QString> get_tools() const {return m_usedTools;}

    //метаданные для логирования
    int get_images_count() const {return m_images.size();}
    int get_tools_count() const {return m_usedTools.size();}

    friend QDataStream &operator<<(QDataStream &out, const TextDeleteData &data)
    {
        out << data.m_position << data.m_length << data.m_images << data.m_usedTools;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in,  TextDeleteData &data)
    {
        in >> data.m_position >>  data.m_length >> data.m_images >> data.m_usedTools;
        return in;
    }

};

}
#endif // PROTOCOL_H

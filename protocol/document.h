#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QDataStream>
#include "protocol.h"

class document_standard
{
    private:
    QString m_text;

    public:

    document_standard() = default;

    explicit document_standard(const QString &initialText) : m_text(initialText) {}

    void applyinsert(const Protocol::TextInsertData &data)
    {
        if(data.get_position() >= 0 && data.get_position() <= m_text.length())
        {
            m_text.insert(data.get_position(), data.get_text());
        }
    }


    void applyDelete(const Protocol::TextDeleteData &data)
    {
        if(data.get_position() >= 0 && data.get_position() + data.get_length() <= m_text.length())
        {
            m_text.remove(data.get_position(), data.get_length());
        }
    }

    QString get_text() const {return m_text;}
    int get_length() const {return m_text.length();}


    friend QDataStream &operator<<(QDataStream &out, const document_standard &doc)
    {
        out << doc.m_text;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, document_standard &doc)
    {
        in >> doc.m_text;
        return in;
    }



};

#endif // DOCUMENT_H

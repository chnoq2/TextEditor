#ifndef STRUCTURES_AND_OTHER_ELEMENTS_H
#define STRUCTURES_AND_OTHER_ELEMENTS_H

#include <QString>
#include <QByteArray>
#include <QDataStream>



struct ImageElement
{
    int index_inside_vector = 0; // позиция изображения внутри вектора -> из std::vector<QString> full text, мы брали индекс у элемента QSTRIN
    QString resolution;
    QByteArray binary_data;

    friend QDataStream &operator<<(QDataStream &out, const ImageElement &img)
    {
        out << img.index_inside_vector << img.resolution << img.binary_data;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, ImageElement &img)
    {
        in >> img.index_inside_vector >> img.resolution >> img.binary_data;
        return in;
    }

};

struct TextStyleElement
{
    int index_inside_vector = 0;
    int length = 0;
    QString font_name;
    bool is_bold = false;
    bool is_static= false;

    friend QDataStream &operator<<(QDataStream &out, const TextStyleElement &style)
    {
        out << style.index_inside_vector << style.length << style.font_name << style.is_bold << style.is_static;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TextStyleElement &style)
    {
        in >> style.index_inside_vector >> style.length >> style.font_name >> style.is_bold >> style.is_static;
        return in;
    }

};


#endif // STRUCTURES_AND_OTHER_ELEMENTS_H

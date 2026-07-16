#ifndef SUPPORRTIVE_STRUCTURES_MODULES_H
#define SUPPORRTIVE_STRUCTURES_MODULES_H


// эти инклуды для трез файлов - document.h protocol.h supportive_structure_modules
#include <QString>
#include <QList>
#include <QMap>
#include <vector>
#include <QByteArray>
#include <QFile>
#include <QtGlobal>

#include <QDataStream>
#include <QXmlStreamReader>
#include <QFileDialog>

#include "qdebug.h"
#include <private/qzipreader_p.h>
#include <private/qzipreader_p.h>

// добавлены новые заголовки
#include <QFile>
#include <QXmlStreamReader>
#include <QFileDialog>
#include <QTimer>


#include <QDebug>

namespace DocAlign {
enum Alignment
    {
    Unknown = -1,
    Left = 0,
    Center = 1,
    Right = 2,
    Justify = 3
    };
}

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

    friend QDebug operator<<(QDebug dbg, const ImageElement &img)
    {
        QDebugStateSaver saver(dbg);
        dbg.nospace() << "ImageElement(Index: " << img.index_inside_vector
                      << ", Res: " << img.resolution
                      << ", Bytes: " << img.binary_data.size() << ")";
        return dbg;
    }

};

struct TextStyleElement
{
    int index_inside_vector = 0;
    int length = 0;
    QString font_name="";
    bool is_bold = false;
    bool is_italic = false;
    bool is_underline = false;

    int font_size =0;

    int alignment = DocAlign::Unknown;

    TextStyleElement() = default;
    TextStyleElement(int start, int len): index_inside_vector(start), length(len){}


    void set_alignment(int align) { alignment = align; }
    void set_bold(bool b) { is_bold = b; }
    void set_italic(bool i) { is_italic = i; }
    void set_underline(bool u) { is_underline = u; }
    void set_font_name(const QString &name) { font_name = name; }
    void set_size(int s) { font_size = s; }

    bool has_formatting() const
    {
        return is_bold || is_italic || is_underline || font_size != 12;
    }

    friend QDataStream &operator<<(QDataStream &out, const TextStyleElement &style)
    {
        out << style.index_inside_vector << style.length << style.font_name << style.is_bold
            << style.is_italic << style.is_underline << style.font_size << style.alignment;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, TextStyleElement &style)
    {
        in >> style.index_inside_vector >> style.length >> style.font_name  >> style.is_bold
            >> style.is_italic>> style.is_underline >> style.font_size >> style.alignment;
        return in;
    }

};


#endif // SUPPORRTIVE_STRUCTURES_MODULES_H

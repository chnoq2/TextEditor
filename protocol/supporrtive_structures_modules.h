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


#endif // SUPPORRTIVE_STRUCTURES_MODULES_H

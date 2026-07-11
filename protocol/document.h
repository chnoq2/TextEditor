#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QList>
#include <QByteArray>
#include <QMap>
#include <QDataStream>
#include <vector>

#include "protocol.h"
#include "qdebug.h"
#include <private/qzipreader_p.h>

// добавлены новые заголовки
#include <QFile>
#include <QXmlStreamReader>
#include <QFileDialog>

#include "structures_and_other_elements.h"

class document_standard
{
private:
    std::vector<QString> full_text;
    QString m_text;
    QString path;
    QMap<int, QList<ImageElement>> m_paragraph_images;
    QMap<int, QList<TextStyleElement>> m_paragraph_styles;

    void update_m_Text()
    {
        m_text = "";
        for(size_t i = 0; i < full_text.size(); ++i) {
            m_text += full_text[i];
            if (i < full_text.size() - 1) {
                m_text += "\n";
            }
        }
    }

public:
    document_standard() = default;

    explicit document_standard(const QString &initialText) {
        m_text = initialText;
        full_text.push_back(initialText);
    }

    void applyinsert(const Protocol::TextInsertData &data)
    {
        int p_idx = data.get_paragraph_index();
        int pos = data.get_position_in_paragraph();

        if(p_idx >= 0 && p_idx < static_cast<int>(full_text.size()))
        {
            if(pos >= 0 && pos <= full_text[p_idx].length())
            {
                full_text[p_idx].insert(pos, data.get_text());

                if(!data.get_images().isEmpty()) {
                    m_paragraph_images[p_idx].append(data.get_images());
                }
                if(!data.get_styles().isEmpty()) {
                    m_paragraph_styles[p_idx].append(data.get_styles());
                }

                update_m_Text();
            }
        }
    }

    void applyDelete(const Protocol::TextDeleteData &data)
    {
        int p_idx = data.get_paragraph_index();
        int pos = data.get_position_in_paragraph();
        int len = data.get_length();

        if(p_idx >= 0 && p_idx < static_cast<int>(full_text.size()))
        {
            if(pos >= 0 && pos + len <= full_text[p_idx].length())
            {
                full_text[p_idx].remove(pos, len);
                update_m_Text();


            }
        }
    }

    QString get_text() const { return m_text; }
    int get_length() const { return m_text.length(); }

    friend QDataStream &operator<<(QDataStream &out, const document_standard &doc)
    {
        out << quint32(doc.full_text.size());
        for(const auto& str : doc.full_text) out << str;
        out << doc.m_paragraph_images << doc.m_paragraph_styles << doc.m_text;
        return out;
    }

    friend QDataStream &operator>>(QDataStream &in, document_standard &doc)
    {
        quint32 size;
        in >> size;
        doc.full_text.resize(size);
        for(quint32 i = 0; i < size; ++i) in >> doc.full_text[i];

        in >> doc.m_paragraph_images >> doc.m_paragraph_styles >> doc.m_text;
        return in;
    }

    void read_doc(const QString &filepath)
    {
        QZipReader zip(filepath);
        if(zip.status() != QZipReader::NoError)
        {
            qDebug() << "[ERROR] This directory doesn`t exist\n";
            return;
        }

        QMap<QString, QByteArray> all_docx_images;
        auto fileList = zip.fileInfoList();

        for (const auto &file : fileList) {
            if (file.filePath.startsWith("word/media/")) {
                QString imageName = file.filePath.section('/', -1);
                all_docx_images[imageName] = zip.fileData(file.filePath);
            }
        }
        QByteArray xmlData = zip.fileData("word/document.xml");
        QXmlStreamReader xml(xmlData);

        full_text.clear();
        m_paragraph_images.clear();
        m_paragraph_styles.clear();

        int current_paragraph_index = 0;
        QString current_paragraph_text = "";
        int current_image_counter = 1;

        while(!xml.atEnd() && !xml.hasError())
        {
            QXmlStreamReader::TokenType token = xml.readNext();

            if(token == QXmlStreamReader::StartElement)
            {
                if(xml.name() == QLatin1String("p"))
                {
                    if(!current_paragraph_text.isEmpty() || current_paragraph_index > 0)
                    {
                        full_text.push_back(current_paragraph_text);
                        current_paragraph_index++;
                        current_paragraph_text = "";
                    }
                }

                if(xml.name() == QLatin1String("t"))
                {
                    current_paragraph_text += xml.readElementText();
                }

                if(xml.name() == QLatin1String("drawing"))
                {
                    ImageElement img;
                    img.index_inside_vector = current_paragraph_text.length();
                    img.resolution = "Auto";

                    QString targetImage = QString("image%1.png").arg(current_image_counter);
                    if (!all_docx_images.contains(targetImage)) {
                        targetImage = QString("image%1.jpeg").arg(current_image_counter);
                    }

                    if(all_docx_images.contains(targetImage))
                    {
                        img.binary_data = all_docx_images[targetImage];
                        m_paragraph_images[current_paragraph_index].append(img);
                        current_image_counter++;
                    }
                }
            }
        }

        if(!current_paragraph_text.isEmpty() || full_text.empty())
        {
            full_text.push_back(current_paragraph_text);
        }

        update_m_Text();
    }
};

#endif // DOCUMENT_H

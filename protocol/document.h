#ifndef DOCUMENT_H
#define DOCUMENT_H

#include "supporrtive_structures_modules.h"
#include "protocol.h"

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
    size_t get_paragraphs_count() const {return full_text.size();}


    const std::vector<QString>& get_full_text() const {return full_text;}

    QList<ImageElement> get_images_for_paragraph(int paragraphIdx) const
    {
        return m_paragraph_images.value(paragraphIdx,QList<ImageElement>());
    }

    QList<TextStyleElement> get_styles_paragraph(int paragrapIdx) const
    {
        return m_paragraph_styles.value(paragrapIdx,QList<TextStyleElement>());
    }


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
            qDebug() << "[ERROR] This directory doesn't exist\n";
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

        int current_paragraph_index = -1;
        QString current_paragraph_text = "";
        int current_image_counter = 1;

        TextStyleElement active_style;
        TextStyleElement paragraph_default_style;

        bool inside_rPr = false;
        bool inside_pPr = false;

        auto readBoolAttribute = [](const QXmlStreamAttributes &attrs) -> bool
        {
            if(!attrs.hasAttribute("w:val")) return true;
            QString val = attrs.value("w:val").toString();
            return (val != "false" && val != "0" && val != "none");
        };

        while(!xml.atEnd() && !xml.hasError())
        {
            QXmlStreamReader::TokenType token = xml.readNext();

            if(token == QXmlStreamReader::StartElement)
            {
                QStringView name = xml.name();
                QXmlStreamAttributes attrs = xml.attributes();

                if(name == QLatin1String("p"))
                {
                    if (current_paragraph_index >= 0) {
                        full_text.push_back(current_paragraph_text);
                    }
                    current_paragraph_index++;
                    current_paragraph_text = "";
                    paragraph_default_style = TextStyleElement();
                }
                else if(name == QLatin1String("pPr"))
                {
                    inside_pPr = true;
                }
                else if(name == QLatin1String("r"))
                {
                    active_style = paragraph_default_style;
                    active_style.index_inside_vector = current_paragraph_text.length();
                }
                else if(name == QLatin1String("rPr"))
                {
                    inside_rPr = true;
                }
                else if(inside_rPr || inside_pPr)
                {
                    TextStyleElement &target_style = inside_pPr ? paragraph_default_style : active_style;

                    if(name == QLatin1String("b"))
                    {
                        target_style.set_bold(readBoolAttribute(attrs));
                    }
                    else if(name == QLatin1String("i"))
                    {
                        target_style.set_italic(readBoolAttribute(attrs));
                    }
                    else if(name == QLatin1String("u"))
                    {
                        target_style.set_underline(readBoolAttribute(attrs));
                    }
                    else if(name == QLatin1String("rFonts"))
                    {
                        if (attrs.hasAttribute("w:ascii")) {
                            target_style.set_font_name(attrs.value("w:ascii").toString());
                        }
                    }
                    else if(name == QLatin1String("sz"))
                    {
                        if (attrs.hasAttribute("w:val"))
                        {
                            int halfPoints = attrs.value("w:val").toInt();
                            if (halfPoints > 0) {
                                target_style.set_size(halfPoints / 2);
                            }
                        }
                    }
                    else if(name == QLatin1String("jc"))
                    {
                        if (attrs.hasAttribute("w:val")) {
                            QString alignVal = attrs.value("w:val").toString();
                            int alignFlag = DocAlign::Unknown;

                            if (alignVal == "center") {
                                alignFlag = DocAlign::Center;
                            } else if (alignVal == "right") {
                                alignFlag = DocAlign::Right;
                            } else if (alignVal == "both") {
                                alignFlag = DocAlign::Justify;
                            } else if (alignVal == "left") {
                                alignFlag = DocAlign::Left;
                            }

                            target_style.set_alignment(alignFlag);
                        }
                    }
                }
                else if(name == QLatin1String("t"))
                {
                    QString runText = xml.readElementText();
                    int startIdx = current_paragraph_text.length();
                    int len = runText.length();

                    current_paragraph_text += runText;

                    active_style.index_inside_vector = startIdx;
                    active_style.length = len;

                    if (current_paragraph_index >= 0)
                    {
                        m_paragraph_styles[current_paragraph_index].append(active_style);
                    }
                }
                else if(name == QLatin1String("drawing"))
                {
                    if (current_paragraph_index >= 0)
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
            else if(token == QXmlStreamReader::EndElement)
            {
                QStringView name = xml.name();
                if(name == QLatin1String("rPr"))
                {
                    inside_rPr = false;
                }
                else if(name == QLatin1String("pPr"))
                {
                  if (current_paragraph_index >= 0 && m_paragraph_styles[current_paragraph_index].isEmpty() && paragraph_default_style.alignment != DocAlign::Unknown) {
                        m_paragraph_styles[current_paragraph_index].append(paragraph_default_style);
                    }
                    inside_pPr = false;
                }
            }
        }

        if(current_paragraph_index >= 0)
        {
            full_text.push_back(current_paragraph_text);
        }
        else if (full_text.empty())
        {
            full_text.push_back(current_paragraph_text);
        }

        update_m_Text();
    }

};
#endif // DOCUMENT_H

#ifndef DOCUMENTEXPORTER_H
#define DOCUMENTEXPORTER_H

#include <QString>

class QTextDocument;

// базовый интерфейс, чтобы добавлять новые форматы (docx), не меняя код, который вызывает сохранение
class DocumentExporter
{
public:
    virtual ~DocumentExporter() = default;
    virtual bool exportDocument(QTextDocument *document, const QString &filePath) = 0;
};

#endif

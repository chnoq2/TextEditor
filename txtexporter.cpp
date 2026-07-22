#include "txtexporter.h"

#include <QTextDocument>
#include <QFile>
#include <QTextStream>

// сохраняет QTextDocument как обычный текстовый файл
bool TxtExporter::exportDocument(QTextDocument *document, const QString &filePath)
{
    if (!document) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) return false;

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << document->toPlainText();

    file.close();
    return true;
}

#ifndef TXTEXPORTER_H
#define TXTEXPORTER_H

#include "documentexporter.h"

class TxtExporter : public DocumentExporter
{
public:
    bool exportDocument(QTextDocument *document, const QString &filePath) override;
};

#endif

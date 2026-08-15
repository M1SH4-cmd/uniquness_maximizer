#ifndef DOCX_READER_H
#define DOCX_READER_H

#include "core/document.h"

class DocxReader
{
public:
    Document read(const QString &path);
    const QString &errorString() const;

private:
    QString m_errorString;
};

#endif // DOCX_READER_H

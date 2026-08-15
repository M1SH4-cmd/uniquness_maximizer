#ifndef DOCUMENT_CHUNK_H
#define DOCUMENT_CHUNK_H

#include <QString>
#include <QVector>

struct ChunkParagraph
{
    QString id;
    QString text;
};

struct DocumentChunk
{
    QString id;
    int index = 0;
    int sectionIndex = 0;
    QVector<QString> paragraphIds;
    QVector<ChunkParagraph> paragraphs;
    QString text;
};

struct ChunkingOptions
{
    int maxCharacters = 6000;
};

#endif // DOCUMENT_CHUNK_H

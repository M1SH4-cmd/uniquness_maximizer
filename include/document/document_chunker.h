#ifndef DOCUMENT_CHUNKER_H
#define DOCUMENT_CHUNKER_H

#include "core/document.h"
#include "core/document_chunk.h"

class DocumentChunker
{
public:
    explicit DocumentChunker(ChunkingOptions options = {});

    QVector<DocumentChunk> split(const Document &document) const;
    const ChunkingOptions &options() const;

private:
    ChunkingOptions m_options;
};

#endif // DOCUMENT_CHUNKER_H

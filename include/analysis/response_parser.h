#ifndef RESPONSE_PARSER_H
#define RESPONSE_PARSER_H

#include "core/analysis_types.h"
#include "core/document_chunk.h"

class ResponseParser
{
public:
    ParseResult parse(const QString &json, const DocumentChunk &chunk) const;
};

#endif // RESPONSE_PARSER_H

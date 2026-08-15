#ifndef PROMPT_BUILDER_H
#define PROMPT_BUILDER_H

#include "core/document_chunk.h"

class PromptBuilder
{
public:
    QString buildSystemPrompt() const;
    QString buildAnalysisPrompt(const DocumentChunk &chunk) const;
};

#endif // PROMPT_BUILDER_H

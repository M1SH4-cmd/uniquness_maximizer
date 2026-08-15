#include "document/document_chunker.h"

namespace {
bool isBlank(const QString &text)
{
    return text.trimmed().isEmpty();
}

DocumentChunk makeChunk(int index, int sectionIndex)
{
    return {QStringLiteral("C%1").arg(index + 1, 3, 10, QLatin1Char('0')), index, sectionIndex, {}, {}, {}};
}
}

DocumentChunker::DocumentChunker(ChunkingOptions options)
    : m_options(options)
{
    if (m_options.maxCharacters < 1) m_options.maxCharacters = 1;
}

QVector<DocumentChunk> DocumentChunker::split(const Document &document) const
{
    QVector<DocumentChunk> chunks;
    DocumentChunk currentChunk;
    bool hasCurrentChunk = false;

    const auto flushCurrentChunk = [&]() {
        if (!hasCurrentChunk) return;
        chunks.append(currentChunk);
        hasCurrentChunk = false;
    };

    for (const Paragraph &paragraph : document.paragraphs()) {
        if (isBlank(paragraph.text)) continue;

        const bool sectionChanged = hasCurrentChunk && currentChunk.sectionIndex != paragraph.sectionIndex;
        const int separatorLength = hasCurrentChunk ? 2 : 0;
        const bool exceedsLimit = hasCurrentChunk
                                  && currentChunk.text.size() + separatorLength + paragraph.text.size() > m_options.maxCharacters;
        if (sectionChanged || exceedsLimit) flushCurrentChunk();

        if (!hasCurrentChunk) {
            currentChunk = makeChunk(chunks.size(), paragraph.sectionIndex);
            hasCurrentChunk = true;
        } else {
            currentChunk.text += QStringLiteral("\n\n");
        }

        currentChunk.paragraphIds.append(paragraph.id);
        currentChunk.paragraphs.append({paragraph.id, paragraph.text});
        currentChunk.text += paragraph.text;
    }
    flushCurrentChunk();
    return chunks;
}

const ChunkingOptions &DocumentChunker::options() const { return m_options; }

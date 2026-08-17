#ifndef TEST_FIXTURES_H
#define TEST_FIXTURES_H

#include "core/document.h"
#include "core/document_chunk.h"

#include <utility>

namespace TestFixtures {

inline Paragraph paragraph(const QString &id, const QString &text, int index, int sectionIndex = 0)
{
    return {id, text, index, sectionIndex, {}};
}

inline Document document(QVector<Paragraph> paragraphs)
{
    return {QStringLiteral("fixture.docx"), QStringLiteral("Fixture"), std::move(paragraphs)};
}

inline DocumentChunk chunk(const QString &id, int index, int sectionIndex, QVector<ChunkParagraph> paragraphs)
{
    QVector<QString> paragraphIds;
    QString text;
    for (const ChunkParagraph &paragraph : paragraphs) {
        paragraphIds.append(paragraph.id);
        if (!text.isEmpty()) text += QStringLiteral("\n\n");
        text += paragraph.text;
    }
    return {id, index, sectionIndex, paragraphIds, std::move(paragraphs), text};
}

}

#endif // TEST_FIXTURES_H

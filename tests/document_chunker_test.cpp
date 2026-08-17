#include "document/document_chunker.h"

#include "test_fixtures.h"
#include "test_support.h"

namespace {
using TestFixtures::document;
using TestFixtures::paragraph;
using TestSupport::require;

bool testSimpleDocument()
{
    const DocumentChunker chunker({100});
    const auto chunks = chunker.split(document({paragraph("P001", "Первый.", 0), paragraph("P002", "Второй.", 1), paragraph("P003", "Третий.", 2)}));
    return require(chunks.size() == 1, QStringLiteral("simple: ожидался один chunk"))
           && require(chunks[0].id == QStringLiteral("C001"), QStringLiteral("simple: неверный ID chunk"))
           && require(chunks[0].paragraphIds == QVector<QString>({"P001", "P002", "P003"}), QStringLiteral("simple: нарушен порядок paragraph IDs"))
           && require(chunks[0].text == QStringLiteral("Первый.\n\nВторой.\n\nТретий."), QStringLiteral("simple: неверный текст chunk"));
}

bool testSizeLimit()
{
    const DocumentChunker chunker({12});
    const auto chunks = chunker.split(document({paragraph("P001", "12345", 0), paragraph("P002", "67890", 1), paragraph("P003", "abcde", 2)}));
    return require(chunks.size() == 2, QStringLiteral("size: следующий абзац должен перейти в новый chunk"))
           && require(chunks[0].paragraphIds == QVector<QString>({"P001", "P002"}), QStringLiteral("size: неверный первый chunk"))
           && require(chunks[1].paragraphIds == QVector<QString>({"P003"}), QStringLiteral("size: неверный второй chunk"));
}

bool testSectionBoundary()
{
    const DocumentChunker chunker({100});
    const auto chunks = chunker.split(document({paragraph("P001", "Один", 0, 0), paragraph("P002", "Два", 1, 0), paragraph("P003", "Три", 2, 1), paragraph("P004", "Четыре", 3, 1)}));
    return require(chunks.size() == 2, QStringLiteral("section: граница секции должна создать новый chunk"))
           && require(chunks[0].sectionIndex == 0 && chunks[1].sectionIndex == 1, QStringLiteral("section: неверные индексы секций"))
           && require(chunks[1].paragraphIds == QVector<QString>({"P003", "P004"}), QStringLiteral("section: абзацы пересекли границу"));
}

bool testLongParagraph()
{
    const QString longText(30, QLatin1Char('x'));
    const DocumentChunker chunker({10});
    const auto chunks = chunker.split(document({paragraph("P017", longText, 0), paragraph("P018", "next", 1)}));
    return require(chunks.size() == 2, QStringLiteral("long: длинный абзац должен быть отдельным chunk"))
           && require(chunks[0].text == longText && chunks[0].paragraphIds == QVector<QString>({"P017"}), QStringLiteral("long: длинный абзац потерян или обрезан"));
}

bool testBlankParagraphs()
{
    const DocumentChunker chunker({100});
    const auto chunks = chunker.split(document({paragraph("P001", "   ", 0), paragraph("P002", "Текст", 1), paragraph("P003", "\n\t", 2)}));
    return require(chunks.size() == 1, QStringLiteral("blank: пустые абзацы создали лишний chunk"))
           && require(chunks[0].paragraphIds == QVector<QString>({"P002"}), QStringLiteral("blank: пустые абзацы попали в chunk"));
}
}

TEST_SUPPORT_MAIN(testSimpleDocument, testSizeLimit, testSectionBoundary, testLongParagraph, testBlankParagraphs)

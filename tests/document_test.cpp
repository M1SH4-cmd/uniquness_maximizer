#include "core/document.h"

#include <QCoreApplication>
#include <QTextStream>

#include <utility>

namespace {
bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

QVector<Paragraph> fixtureParagraphs()
{
    return {{QStringLiteral("P001"), QStringLiteral("Введение"), 0, 0, QStringLiteral("Heading1")},
            {QStringLiteral("P002"), QStringLiteral("Первый абзац."), 1, 1, QString()}};
}

bool testDefaultConstructed()
{
    const Document document;
    return require(document.isEmpty(), QStringLiteral("default: пустой документ не отмечен пустым"))
           && require(document.filePath().isEmpty() && document.title().isEmpty(), QStringLiteral("default: путь или заголовок не пусты"))
           && require(document.paragraphs().isEmpty(), QStringLiteral("default: список абзацев не пуст"));
}

bool testAccessors()
{
    const Document document(QStringLiteral("C:/work/diploma.docx"), QStringLiteral("diploma.docx"), fixtureParagraphs());
    return require(document.filePath() == QStringLiteral("C:/work/diploma.docx"), QStringLiteral("accessors: путь искажён"))
           && require(document.title() == QStringLiteral("diploma.docx"), QStringLiteral("accessors: заголовок искажён"))
           && require(!document.isEmpty(), QStringLiteral("accessors: документ с абзацами отмечен пустым"))
           && require(document.paragraphs().size() == 2, QStringLiteral("accessors: потерян абзац"))
           && require(document.paragraphs().at(0).style == QStringLiteral("Heading1"), QStringLiteral("accessors: стиль абзаца потерян"))
           && require(document.paragraphs().at(1).index == 1 && document.paragraphs().at(1).sectionIndex == 1,
                      QStringLiteral("accessors: индексы абзаца искажены"));
}

bool testEmptyParagraphListIsEmptyDocument()
{
    const Document document(QStringLiteral("empty.docx"), QStringLiteral("empty.docx"), {});
    return require(document.isEmpty(), QStringLiteral("empty: документ без абзацев не отмечен пустым"))
           && require(document.filePath() == QStringLiteral("empty.docx"), QStringLiteral("empty: путь потерян"));
}

bool testCopyAndMovePreserveContent()
{
    Document source(QStringLiteral("source.docx"), QStringLiteral("source.docx"), fixtureParagraphs());
    const Document copy = source;
    const Document moved = std::move(source);
    return require(copy.paragraphs().size() == 2 && copy.title() == QStringLiteral("source.docx"),
                   QStringLiteral("copy: копия потеряла содержимое"))
           && require(moved.paragraphs().size() == 2 && moved.filePath() == QStringLiteral("source.docx"),
                      QStringLiteral("move: перемещённый документ потерял содержимое"))
           && require(moved.paragraphs().at(0).id == QStringLiteral("P001"), QStringLiteral("move: порядок абзацев нарушен"));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    return testDefaultConstructed() && testAccessors() && testEmptyParagraphListIsEmptyDocument()
           && testCopyAndMovePreserveContent() ? 0 : 1;
}

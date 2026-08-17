#include "document/docx_reader.h"

#include "test_support.h"

#include <QDir>
#include <QFile>
#include <QTemporaryDir>

#include <zip.h>

namespace {
using TestSupport::fail;

bool addEntry(zip_t *archive, const char *name, const QByteArray &data)
{
    zip_source_t *source = zip_source_buffer(archive, data.constData(), static_cast<zip_uint64_t>(data.size()), 0);
    return source && zip_file_add(archive, name, source, ZIP_FL_ENC_UTF_8) >= 0;
}

bool createFixture(const QString &path)
{
    int error = 0;
    zip_t *archive = zip_open(QFile::encodeName(path).constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!archive) return false;
    const QByteArray contentTypes = R"(<?xml version="1.0"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="xml" ContentType="application/xml"/></Types>)";
    const QByteArray documentXml = R"(<?xml version="1.0" encoding="UTF-8"?><w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"><w:body><w:p><w:pPr><w:pStyle w:val="Heading1"/></w:pPr><w:r><w:t>Введение</w:t></w:r></w:p><w:p><w:r><w:t>Первый абзац.</w:t></w:r></w:p><w:p><w:r><w:t>Второй абзац</w:t></w:r><w:r><w:tab/><w:t>с табуляцией.</w:t></w:r></w:p><w:p><w:pPr><w:pStyle w:val="Heading1"/></w:pPr><w:r><w:t>Заключение</w:t></w:r></w:p></w:body></w:document>)";
    const bool added = addEntry(archive, "[Content_Types].xml", contentTypes)
                       && addEntry(archive, "word/document.xml", documentXml);
    return added && zip_close(archive) == 0;
}

}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir tempDirectory;
    if (!tempDirectory.isValid()) return fail(QStringLiteral("Не удалось создать временную папку."));
    const QString fixturePath = QDir(tempDirectory.path()).filePath(QStringLiteral("reader_fixture.docx"));
    if (!createFixture(fixturePath)) return fail(QStringLiteral("Не удалось создать DOCX-фикстуру."));

    DocxReader reader;
    const Document document = reader.read(fixturePath);
    if (!reader.errorString().isEmpty()) return fail(reader.errorString());
    if (document.paragraphs().size() != 4) return fail(QStringLiteral("Ожидалось 4 абзаца."));
    if (document.paragraphs().at(0).text != QStringLiteral("Введение")
        || document.paragraphs().at(1).text != QStringLiteral("Первый абзац.")
        || document.paragraphs().at(2).text != QStringLiteral("Второй абзац\tс табуляцией.")) {
        return fail(QStringLiteral("Порядок или текст абзацев извлечён неверно."));
    }
    if (document.paragraphs().at(0).style != QStringLiteral("Heading1")
        || document.paragraphs().at(3).sectionIndex <= document.paragraphs().at(2).sectionIndex) {
        return fail(QStringLiteral("Стиль или секции извлечены неверно."));
    }
    return 0;
}

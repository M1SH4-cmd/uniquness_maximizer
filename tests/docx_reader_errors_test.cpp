#include "document/docx_reader.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QTextStream>

#include <zip.h>

namespace {
bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

bool addEntry(zip_t *archive, const char *name, const QByteArray &data)
{
    zip_source_t *source = zip_source_buffer(archive, data.constData(), static_cast<zip_uint64_t>(data.size()), 0);
    return source && zip_file_add(archive, name, source, ZIP_FL_ENC_UTF_8) >= 0;
}

// Writes a ZIP archive containing exactly the given entries, so that individual
// DOCX defects (missing part, empty part, broken XML) can be reproduced.
bool createArchive(const QString &path, const QVector<QPair<QByteArray, QByteArray>> &entries)
{
    int error = 0;
    zip_t *archive = zip_open(QFile::encodeName(path).constData(), ZIP_CREATE | ZIP_TRUNCATE, &error);
    if (!archive) return false;
    for (const auto &entry : entries) {
        if (!addEntry(archive, entry.first.constData(), entry.second)) {
            zip_discard(archive);
            return false;
        }
    }
    return zip_close(archive) == 0;
}

bool writeFile(const QString &path, const QByteArray &content)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) return false;
    return file.write(content) == content.size();
}

QString documentXml(const QByteArray &body)
{
    return QStringLiteral(R"(<?xml version="1.0" encoding="UTF-8"?><w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main">%1</w:document>)")
        .arg(QString::fromUtf8(body));
}

bool testMissingFile(const QDir &directory)
{
    DocxReader reader;
    const Document document = reader.read(directory.filePath(QStringLiteral("does_not_exist.docx")));
    return require(document.isEmpty(), QStringLiteral("missing: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("не найден")), QStringLiteral("missing: ошибка не описывает отсутствие файла"));
}

bool testDirectoryInsteadOfFile(const QDir &directory)
{
    DocxReader reader;
    const Document document = reader.read(directory.absolutePath());
    return require(document.isEmpty() && !reader.errorString().isEmpty(), QStringLiteral("directory: папка принята как DOCX"));
}

bool testNotAZipArchive(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("plain.docx"));
    if (!require(writeFile(path, QByteArrayLiteral("это обычный текст, а не архив")), QStringLiteral("not zip: не удалось создать файл"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty(), QStringLiteral("not zip: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("ZIP")), QStringLiteral("not zip: ошибка не описывает проблему архива"));
}

bool testMissingDocumentPart(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("no_document.docx"));
    if (!require(createArchive(path, {{"[Content_Types].xml", QByteArrayLiteral("<Types/>")}}), QStringLiteral("no part: не удалось создать архив"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty(), QStringLiteral("no part: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("word/document.xml")), QStringLiteral("no part: ошибка не указывает отсутствующую часть"));
}

bool testEmptyDocumentPart(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("empty_document.docx"));
    if (!require(createArchive(path, {{"word/document.xml", QByteArray()}}), QStringLiteral("empty part: не удалось создать архив"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty() && !reader.errorString().isEmpty(), QStringLiteral("empty part: пустой word/document.xml принят"));
}

bool testMalformedXml(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("broken_xml.docx"));
    if (!require(createArchive(path, {{"word/document.xml", QByteArrayLiteral("<w:document><w:body>")}}), QStringLiteral("broken xml: не удалось создать архив"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty(), QStringLiteral("broken xml: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("разобрать")), QStringLiteral("broken xml: ошибка не описывает разбор XML"));
}

bool testMissingBody(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("no_body.docx"));
    if (!require(createArchive(path, {{"word/document.xml", documentXml(QByteArrayLiteral("<w:notBody/>")).toUtf8()}}),
                 QStringLiteral("no body: не удалось создать архив"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty(), QStringLiteral("no body: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("тело")), QStringLiteral("no body: ошибка не описывает отсутствие тела"));
}

bool testDocumentWithoutParagraphs(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("no_paragraphs.docx"));
    if (!require(createArchive(path, {{"word/document.xml", documentXml(QByteArrayLiteral("<w:body><w:tbl/><w:sectPr/></w:body>")).toUtf8()}}),
                 QStringLiteral("no paragraphs: не удалось создать архив"))) return false;
    DocxReader reader;
    const Document document = reader.read(path);
    return require(document.isEmpty(), QStringLiteral("no paragraphs: возвращён непустой документ"))
           && require(reader.errorString().contains(QStringLiteral("абзац")), QStringLiteral("no paragraphs: ошибка не описывает отсутствие абзацев"));
}

bool testTextExtractionDetails(const QDir &directory)
{
    const QString path = directory.filePath(QStringLiteral("rich_text.docx"));
    const QByteArray body = QByteArrayLiteral(
        "<w:body>"
        "<w:p><w:pPr><w:outlineLvl w:val=\"0\"/></w:pPr><w:r><w:t>Обзор источников</w:t></w:r></w:p>"
        "<w:p><w:r><w:t>Первая строка</w:t><w:br/><w:t>вторая строка</w:t></w:r></w:p>"
        "<w:p><w:r><w:delText>Удалённый фрагмент</w:delText><w:cr/><w:t>после переноса</w:t></w:r></w:p>"
        "<w:p><w:pPr><w:pStyle w:val=\"Заголовок 2\"/></w:pPr><w:r><w:t>Выводы</w:t></w:r></w:p>"
        "</w:body>");
    if (!require(createArchive(path, {{"word/document.xml", documentXml(body).toUtf8()}}), QStringLiteral("rich: не удалось создать архив"))) return false;

    DocxReader reader;
    const Document document = reader.read(path);
    if (!require(reader.errorString().isEmpty(), QStringLiteral("rich: %1").arg(reader.errorString()))) return false;
    if (!require(document.paragraphs().size() == 4, QStringLiteral("rich: ожидалось 4 абзаца"))) return false;
    const QVector<Paragraph> paragraphs = document.paragraphs();
    return require(paragraphs.at(0).id == QStringLiteral("P0001") && paragraphs.at(3).id == QStringLiteral("P0004"),
                   QStringLiteral("rich: идентификаторы абзацев сформированы неверно"))
           && require(paragraphs.at(1).text == QStringLiteral("Первая строка\nвторая строка"), QStringLiteral("rich: w:br не преобразован в перенос строки"))
           && require(paragraphs.at(2).text == QStringLiteral("Удалённый фрагмент\nпосле переноса"), QStringLiteral("rich: w:delText или w:cr обработаны неверно"))
           && require(paragraphs.at(0).sectionIndex == 0 && paragraphs.at(3).sectionIndex == 1,
                      QStringLiteral("rich: заголовок по outlineLvl или русскому стилю не открыл новую секцию"))
           && require(document.title() == QStringLiteral("rich_text"), QStringLiteral("rich: заголовок документа не взят из имени файла"));
}

bool testErrorStringResetBetweenReads(const QDir &directory)
{
    const QString brokenPath = directory.filePath(QStringLiteral("reset_broken.docx"));
    const QString validPath = directory.filePath(QStringLiteral("reset_valid.docx"));
    const bool prepared = writeFile(brokenPath, QByteArrayLiteral("not a zip"))
                          && createArchive(validPath, {{"word/document.xml", documentXml(QByteArrayLiteral("<w:body><w:p><w:r><w:t>Абзац.</w:t></w:r></w:p></w:body>")).toUtf8()}});
    if (!require(prepared, QStringLiteral("reset: не удалось подготовить файлы"))) return false;

    DocxReader reader;
    reader.read(brokenPath);
    if (!require(!reader.errorString().isEmpty(), QStringLiteral("reset: ошибка не зафиксирована"))) return false;
    const Document document = reader.read(validPath);
    return require(reader.errorString().isEmpty(), QStringLiteral("reset: ошибка не сброшена при успешном чтении"))
           && require(document.paragraphs().size() == 1, QStringLiteral("reset: корректный документ прочитан неверно"));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    QTemporaryDir tempDirectory;
    if (!require(tempDirectory.isValid(), QStringLiteral("Не удалось создать временную папку."))) return 1;
    const QDir directory(tempDirectory.path());
    return testMissingFile(directory) && testDirectoryInsteadOfFile(directory) && testNotAZipArchive(directory)
           && testMissingDocumentPart(directory) && testEmptyDocumentPart(directory) && testMalformedXml(directory)
           && testMissingBody(directory) && testDocumentWithoutParagraphs(directory)
           && testTextExtractionDetails(directory) && testErrorStringResetBetweenReads(directory) ? 0 : 1;
}

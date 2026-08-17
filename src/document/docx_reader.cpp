#include "document/docx_reader.h"

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

#include <zip.h>
#include <pugixml.hpp>

namespace {
constexpr zip_uint64_t kMaxXmlSize = 64ULL * 1024ULL * 1024ULL;

QByteArray readZipEntry(zip_t *archive, const char *entryName, QString &error)
{
    zip_stat_t stat;
    zip_stat_init(&stat);
    if (zip_stat(archive, entryName, ZIP_FL_ENC_GUESS, &stat) != 0) {
        error = QStringLiteral("В DOCX отсутствует %1.").arg(QString::fromLatin1(entryName));
        return {};
    }
    if (stat.size == 0 || stat.size > kMaxXmlSize) {
        error = QStringLiteral("Некорректный или слишком большой XML-файл %1.").arg(QString::fromLatin1(entryName));
        return {};
    }

    zip_file_t *entry = zip_fopen(archive, entryName, ZIP_FL_ENC_GUESS);
    if (!entry) {
        error = QStringLiteral("Не удалось открыть %1 в DOCX.").arg(QString::fromLatin1(entryName));
        return {};
    }

    QByteArray data(static_cast<qsizetype>(stat.size), Qt::Uninitialized);
    const zip_int64_t bytesRead = zip_fread(entry, data.data(), stat.size);
    zip_fclose(entry);
    if (bytesRead != static_cast<zip_int64_t>(stat.size)) {
        error = QStringLiteral("Не удалось полностью прочитать %1.").arg(QString::fromLatin1(entryName));
        return {};
    }
    return data;
}

bool nodeNameIs(const pugi::xml_node &node, const char *name)
{
    return QLatin1String(node.name()) == QLatin1String(name);
}

pugi::xml_node childByName(const pugi::xml_node &node, const char *name)
{
    for (const pugi::xml_node &child : node.children()) {
        if (nodeNameIs(child, name)) return child;
    }
    return {};
}

QString attributeByName(const pugi::xml_node &node, const char *name)
{
    for (const pugi::xml_attribute &attribute : node.attributes()) {
        if (QString::fromLatin1(attribute.name()) == QLatin1String(name)) {
            return QString::fromUtf8(attribute.value());
        }
    }
    return {};
}

void appendText(const pugi::xml_node &node, QString &text)
{
    if (nodeNameIs(node, "w:t") || nodeNameIs(node, "w:delText")) {
        text += QString::fromUtf8(node.text().as_string());
        return;
    }
    if (nodeNameIs(node, "w:tab")) {
        text += QLatin1Char('\t');
        return;
    }
    if (nodeNameIs(node, "w:br") || nodeNameIs(node, "w:cr")) {
        text += QLatin1Char('\n');
        return;
    }
    for (const pugi::xml_node &child : node.children()) appendText(child, text);
}

bool isHeading(const QString &style, const pugi::xml_node &paragraphProperties)
{
    if (childByName(paragraphProperties, "w:outlineLvl")) return true;
    return style.contains(QRegularExpression(QStringLiteral("^(heading|заголовок)"), QRegularExpression::CaseInsensitiveOption));
}
}

Document DocxReader::read(const QString &path)
{
    m_errorString.clear();
    const QFileInfo fileInfo(path);
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        m_errorString = QStringLiteral("Файл DOCX не найден.");
        return {};
    }

    int zipError = 0;
    zip_t *archive = zip_open(QFile::encodeName(path).constData(), ZIP_RDONLY, &zipError);
    if (!archive) {
        m_errorString = QStringLiteral("Не удалось открыть DOCX как ZIP-архив (ошибка %1).").arg(zipError);
        return {};
    }

    const QByteArray xmlData = readZipEntry(archive, "word/document.xml", m_errorString);
    zip_close(archive);
    if (xmlData.isEmpty()) return {};

    pugi::xml_document xml;
    const pugi::xml_parse_result parseResult = xml.load_buffer(xmlData.constData(), static_cast<size_t>(xmlData.size()));
    if (!parseResult) {
        m_errorString = QStringLiteral("Не удалось разобрать word/document.xml: %1.")
                            .arg(QString::fromLatin1(parseResult.description()));
        return {};
    }

    QVector<Paragraph> paragraphs;
    int sectionIndex = 0;
    const pugi::xml_node body = childByName(childByName(xml, "w:document"), "w:body");
    if (!body) {
        m_errorString = QStringLiteral("В DOCX не найдено тело документа.");
        return {};
    }

    for (const pugi::xml_node &node : body.children()) {
        if (!nodeNameIs(node, "w:p")) continue;

        const pugi::xml_node properties = childByName(node, "w:pPr");
        const pugi::xml_node styleNode = childByName(properties, "w:pStyle");
        const QString style = attributeByName(styleNode, "w:val");
        if (isHeading(style, properties) && !paragraphs.isEmpty()) ++sectionIndex;

        QString text;
        appendText(node, text);
        const int index = paragraphs.size();
        paragraphs.append({QStringLiteral("P%1").arg(index + 1, 4, 10, QLatin1Char('0')), text, index, sectionIndex, style});
    }

    if (paragraphs.isEmpty()) {
        m_errorString = QStringLiteral("В DOCX не найдено ни одного абзаца.");
        return {};
    }
    return {fileInfo.absoluteFilePath(), fileInfo.completeBaseName(), paragraphs};
}

const QString &DocxReader::errorString() const { return m_errorString; }

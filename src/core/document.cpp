#include "core/document.h"

#include <utility>

Document::Document(QString filePath, QString title, QVector<Paragraph> paragraphs)
    : m_filePath(std::move(filePath)),
      m_title(std::move(title)),
      m_paragraphs(std::move(paragraphs))
{
}

const QString &Document::filePath() const { return m_filePath; }
const QString &Document::title() const { return m_title; }
const QVector<Paragraph> &Document::paragraphs() const { return m_paragraphs; }
bool Document::isEmpty() const { return m_paragraphs.isEmpty(); }

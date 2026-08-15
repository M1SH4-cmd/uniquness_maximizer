#ifndef DOCUMENT_H
#define DOCUMENT_H

#include <QString>
#include <QVector>

struct Paragraph
{
    QString id;
    QString text;
    int index = 0;
    int sectionIndex = 0;
    QString style;
};

class Document
{
public:
    Document() = default;
    Document(QString filePath, QString title, QVector<Paragraph> paragraphs);

    const QString &filePath() const;
    const QString &title() const;
    const QVector<Paragraph> &paragraphs() const;
    bool isEmpty() const;

private:
    QString m_filePath;
    QString m_title;
    QVector<Paragraph> m_paragraphs;
};

#endif // DOCUMENT_H

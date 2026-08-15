#ifndef ANALYSIS_TYPES_H
#define ANALYSIS_TYPES_H

#include <QString>
#include <QVector>

enum class Severity { Low, Medium, High };

enum class IssueCategory {
    Style,
    Repetition,
    UnsupportedClaim,
    Logic,
    Citation,
    AcademicStyle,
    Terminology,
    Structure,
    Other
};

struct Issue
{
    QString id;
    QString paragraphId;
    IssueCategory category = IssueCategory::Other;
    Severity severity = Severity::Low;
    double confidence = 0.0;
    QString originalText;
    QString problem;
    QString recommendation;
    QString suggestedText;
};

struct AnalysisResult
{
    QVector<Issue> issues;
    struct ChunkError {
        QString chunkId;
        QString message;
    };
    QVector<ChunkError> errors;
    int processedParagraphs = 0;
    int processedChunks = 0;
    bool cancelled = false;

    bool isPartial() const { return !errors.isEmpty() || cancelled; }
};

struct ParseResult
{
    bool success = false;
    AnalysisResult result;
    QString errorMessage;
};

#endif // ANALYSIS_TYPES_H

#include "analysis/response_parser.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QHash>
#include <QSet>

namespace {
bool requiredString(const QJsonObject &object, const char *name, QString &value, QString &error)
{
    const QJsonValue jsonValue = object.value(QLatin1String(name));
    if (!jsonValue.isString() || (value = jsonValue.toString()).isEmpty()) {
        error = QStringLiteral("Поле '%1' обязательно и должно быть непустой строкой.").arg(QLatin1String(name));
        return false;
    }
    return true;
}

bool categoryFromString(const QString &value, IssueCategory &category)
{
    static const QHash<QString, IssueCategory> values{{"style", IssueCategory::Style}, {"repetition", IssueCategory::Repetition}, {"unsupported_claim", IssueCategory::UnsupportedClaim}, {"logic", IssueCategory::Logic}, {"citation", IssueCategory::Citation}, {"academic_style", IssueCategory::AcademicStyle}, {"terminology", IssueCategory::Terminology}, {"structure", IssueCategory::Structure}, {"other", IssueCategory::Other}};
    if (!values.contains(value)) return false;
    category = values.value(value);
    return true;
}

bool severityFromString(const QString &value, Severity &severity)
{
    if (value == QLatin1String("low")) severity = Severity::Low;
    else if (value == QLatin1String("medium")) severity = Severity::Medium;
    else if (value == QLatin1String("high")) severity = Severity::High;
    else return false;
    return true;
}
}

ParseResult ResponseParser::parse(const QString &json, const DocumentChunk &chunk) const
{
    ParseResult parsed;
    if (json.trimmed().isEmpty()) {
        parsed.errorMessage = QStringLiteral("Ответ модели пуст.");
        return parsed;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        parsed.errorMessage = QStringLiteral("Ответ модели содержит некорректный JSON: %1.").arg(parseError.errorString());
        return parsed;
    }
    const QJsonValue issuesValue = document.object().value(QStringLiteral("issues"));
    if (!issuesValue.isArray()) {
        parsed.errorMessage = QStringLiteral("Корневой объект должен содержать массив 'issues'.");
        return parsed;
    }

    QSet<QString> allowedParagraphIds;
    for (const QString &id : chunk.paragraphIds) allowedParagraphIds.insert(id);
    AnalysisResult result;
    result.processedChunks = 1;
    result.processedParagraphs = chunk.paragraphIds.size();
    const QJsonArray issues = issuesValue.toArray();
    for (qsizetype index = 0; index < issues.size(); ++index) {
        if (!issues.at(index).isObject()) {
            parsed.errorMessage = QStringLiteral("Элемент issues[%1] должен быть объектом.").arg(index);
            return parsed;
        }
        const QJsonObject object = issues.at(index).toObject();
        Issue issue;
        QString error;
        QString category;
        if (!requiredString(object, "id", issue.id, error) || !requiredString(object, "paragraph_id", issue.paragraphId, error)
            || !requiredString(object, "category", category, error)) {
            parsed.errorMessage = QStringLiteral("issues[%1]: %2").arg(index).arg(error);
            return parsed;
        }
        QString severity;
        if (!categoryFromString(category, issue.category)) {
            parsed.errorMessage = QStringLiteral("issues[%1]: неизвестная category '%2'.").arg(index).arg(category);
            return parsed;
        }
        if (!requiredString(object, "severity", severity, error) || !severityFromString(severity, issue.severity)
            || !requiredString(object, "original", issue.originalText, error) || !requiredString(object, "problem", issue.problem, error)
            || !requiredString(object, "recommendation", issue.recommendation, error)) {
            parsed.errorMessage = QStringLiteral("issues[%1]: %2").arg(index).arg(error.isEmpty() ? QStringLiteral("неизвестная severity '%1'.").arg(severity) : error);
            return parsed;
        }
        if (!allowedParagraphIds.contains(issue.paragraphId)) {
            parsed.errorMessage = QStringLiteral("issues[%1]: paragraph_id '%2' не принадлежит chunk %3.").arg(index).arg(issue.paragraphId, chunk.id);
            return parsed;
        }
        const QJsonValue confidenceValue = object.value(QStringLiteral("confidence"));
        if (!confidenceValue.isDouble() || (issue.confidence = confidenceValue.toDouble()) < 0.0 || issue.confidence > 1.0) {
            parsed.errorMessage = QStringLiteral("issues[%1]: confidence должен быть числом от 0.0 до 1.0.").arg(index);
            return parsed;
        }
        const QJsonValue suggestedText = object.value(QStringLiteral("suggested_text"));
        if (!suggestedText.isUndefined() && !suggestedText.isString()) {
            parsed.errorMessage = QStringLiteral("issues[%1]: suggested_text должен быть строкой.").arg(index);
            return parsed;
        }
        issue.suggestedText = suggestedText.toString();
        result.issues.append(issue);
    }
    parsed.success = true;
    parsed.result = result;
    return parsed;
}

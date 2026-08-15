#include "analysis/prompt_builder.h"
#include "analysis/response_parser.h"

#include <QCoreApplication>
#include <QTextStream>

namespace {
DocumentChunk fixtureChunk()
{
    return {"C001", 0, 2, {"P001", "P002", "P003"},
            {{"P001", "Первый текст."}, {"P002", "Исходный текст"}, {"P003", "Третий текст."}},
            "Первый текст.\n\nИсходный текст\n\nТретий текст."};
}

bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

QString issueJson(const QString &overrides = {})
{
    return QStringLiteral(R"({"issues":[{"id":"ISSUE_001","paragraph_id":"P002","category":"style","severity":"medium","confidence":0.87,"original":"Исходный текст","problem":"Формулировка недостаточно ясна.","recommendation":"Сделать формулировку конкретнее.","suggested_text":"Более конкретная формулировка."%1}]})").arg(overrides);
}

bool testValidJson()
{
    const ParseResult parsed = ResponseParser().parse(issueJson(), fixtureChunk());
    return require(parsed.success, QStringLiteral("valid: %1").arg(parsed.errorMessage))
           && require(parsed.result.issues.size() == 1, QStringLiteral("valid: issue не создан"))
           && require(parsed.result.issues[0].category == IssueCategory::Style && parsed.result.issues[0].severity == Severity::Medium, QStringLiteral("valid: enum преобразован неверно"));
}

bool testEmptyIssues()
{
    const ParseResult parsed = ResponseParser().parse(QStringLiteral("{\"issues\":[]}"), fixtureChunk());
    return require(parsed.success && parsed.result.issues.isEmpty(), QStringLiteral("empty: пустой массив не принят"));
}

bool testInvalidResponses()
{
    const ResponseParser parser;
    return require(!parser.parse(QStringLiteral("not json"), fixtureChunk()).success, QStringLiteral("invalid: некорректный JSON принят"))
           && require(!parser.parse(QStringLiteral("{}"), fixtureChunk()).success, QStringLiteral("invalid: issues отсутствует"))
           && require(!parser.parse(QStringLiteral("{\"issues\":\"bad\"}"), fixtureChunk()).success, QStringLiteral("invalid: неверный тип issues принят"));
}

bool testEnumAndParagraphValidation()
{
    const ResponseParser parser;
    QString json = issueJson();
    json.replace(QStringLiteral("\"style\""), QStringLiteral("\"unknown\""));
    if (!require(!parser.parse(json, fixtureChunk()).success, QStringLiteral("enum: неизвестная category принята"))) return false;
    json = issueJson();
    json.replace(QStringLiteral("\"medium\""), QStringLiteral("\"critical\""));
    if (!require(!parser.parse(json, fixtureChunk()).success, QStringLiteral("enum: неизвестная severity принята"))) return false;
    json = issueJson();
    json.replace(QStringLiteral("\"P002\""), QStringLiteral("\"P999\""));
    return require(!parser.parse(json, fixtureChunk()).success, QStringLiteral("paragraph: чужой paragraph_id принят"));
}

bool testRequiredFieldsAndConfidence()
{
    const ResponseParser parser;
    QString json = issueJson();
    json.replace(QStringLiteral("\"problem\":\"Формулировка недостаточно ясна.\","), QString());
    if (!require(!parser.parse(json, fixtureChunk()).success, QStringLiteral("required: отсутствие problem принято"))) return false;
    for (const QString &invalid : {QStringLiteral("-0.1"), QStringLiteral("1.5")}) {
        json = issueJson();
        json.replace(QStringLiteral("0.87"), invalid);
        if (!require(!parser.parse(json, fixtureChunk()).success, QStringLiteral("confidence: выход за диапазон принят"))) return false;
    }
    for (const QString &valid : {QStringLiteral("0.0"), QStringLiteral("0.5"), QStringLiteral("1.0")}) {
        json = issueJson();
        json.replace(QStringLiteral("0.87"), valid);
        if (!require(parser.parse(json, fixtureChunk()).success, QStringLiteral("confidence: корректное значение отклонено"))) return false;
    }
    json = issueJson();
    json.replace(QStringLiteral(",\"suggested_text\":\"Более конкретная формулировка.\""), QString());
    const ParseResult withoutSuggestedText = parser.parse(json, fixtureChunk());
    return require(withoutSuggestedText.success && withoutSuggestedText.result.issues[0].suggestedText.isEmpty(), QStringLiteral("suggested_text: необязательное поле обработано неверно"));
}

bool testPromptBuilder()
{
    const PromptBuilder builder;
    const QString system = builder.buildSystemPrompt();
    const QString user = builder.buildAnalysisPrompt(fixtureChunk());
    return require(system.contains(QStringLiteral("JSON")), QStringLiteral("prompt: system prompt не требует JSON"))
           && require(user.contains(QStringLiteral("C001")) && user.contains(QStringLiteral("SECTION: 2")), QStringLiteral("prompt: нет chunk/section"))
           && require(user.contains(QStringLiteral("[P001]")) && user.contains(QStringLiteral("Исходный текст")), QStringLiteral("prompt: нет paragraph ID или текста"));
}
}

int main(int argc, char *argv[])
{
    QCoreApplication application(argc, argv);
    return testValidJson() && testEmptyIssues() && testInvalidResponses() && testEnumAndParagraphValidation()
           && testRequiredFieldsAndConfidence() && testPromptBuilder() ? 0 : 1;
}

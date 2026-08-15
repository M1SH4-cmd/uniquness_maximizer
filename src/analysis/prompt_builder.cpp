#include "analysis/prompt_builder.h"

#include <QStringList>

QString PromptBuilder::buildSystemPrompt() const
{
    return QStringLiteral(
        "Ты — научный редактор. Анализируй только предоставленный фрагмент академического текста. "
        "Ищи проблемы академического стиля, ясности, логической связности, повторов, терминологии, "
        "переходов и структуры аргументации; чрезмерно категоричные, общие или неподтверждённые утверждения; "
        "места, где нужны источник или собственный анализ. Не придумывай факты, источники и библиографические ссылки. "
        "Если утверждение требует источника, укажи: «Требуется проверить/добавить источник». "
        "Не переписывай весь текст и не создавай замечания без основания. "
        "Верни только корректный JSON без Markdown и пояснений. Формат: "
        "{\"issues\":[{\"id\":\"ISSUE_001\",\"paragraph_id\":\"P001\",\"category\":\"style\","
        "\"severity\":\"low\",\"confidence\":0.0,\"original\":\"\",\"problem\":\"\","
        "\"recommendation\":\"\",\"suggested_text\":\"\"}]}. "
        "Допустимые category: style, repetition, unsupported_claim, logic, citation, academic_style, terminology, structure, other. "
        "Допустимые severity: low, medium, high. confidence — число от 0.0 до 1.0. suggested_text необязателен.");
}

QString PromptBuilder::buildAnalysisPrompt(const DocumentChunk &chunk) const
{
    QStringList lines;
    lines << QStringLiteral("CHUNK: %1").arg(chunk.id)
          << QStringLiteral("SECTION: %1").arg(chunk.sectionIndex)
          << QStringLiteral("Идентификаторы абзацев технические: не интерпретируй их как оценку качества или важности.");
    for (const ChunkParagraph &paragraph : chunk.paragraphs) {
        lines << QStringLiteral("\n[%1]\n%2").arg(paragraph.id, paragraph.text);
    }
    lines << QStringLiteral("\nВерни только JSON в согласованном формате.");
    return lines.join(QLatin1Char('\n'));
}

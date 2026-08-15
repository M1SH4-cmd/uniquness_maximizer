#ifndef AI_TYPES_H
#define AI_TYPES_H

#include <QString>

struct AIRequest
{
    QString systemPrompt;
    QString userPrompt;
    QString model;
    double temperature = 0.2;
};

struct AIResponse
{
    bool success = false;
    QString rawText;
    QString errorMessage;
};

#endif // AI_TYPES_H

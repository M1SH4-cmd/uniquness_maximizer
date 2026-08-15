#ifndef AI_PROVIDER_H
#define AI_PROVIDER_H

#include "core/ai_types.h"

#include <functional>

class AIProvider
{
public:
    virtual ~AIProvider() = default;

    virtual void analyze(const AIRequest &request, std::function<void(AIResponse)> callback) = 0;
    virtual QString providerName() const = 0;
};

#endif // AI_PROVIDER_H

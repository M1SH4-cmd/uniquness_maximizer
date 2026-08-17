#ifndef ASYNC_TEST_SUPPORT_H
#define ASYNC_TEST_SUPPORT_H

#include "core/ai_types.h"
#include "providers/ai_provider.h"

#include <QEventLoop>
#include <QTimer>

#include <functional>
#include <utility>

namespace TestSupport {

// Runs `invoker`, which must call the passed `done` callback once the awaited
// asynchronous work has completed, and returns when `done` was called or the
// timeout expired. Callbacks invoked synchronously are handled without
// entering the event loop.
template <typename Invoker>
inline void awaitCallback(int timeoutMs, Invoker &&invoker)
{
    QEventLoop loop;
    bool finished = false;
    std::forward<Invoker>(invoker)(std::function<void()>([&finished, &loop]() {
        finished = true;
        loop.quit();
    }));
    if (finished) return;
    QTimer::singleShot(timeoutMs, &loop, &QEventLoop::quit);
    loop.exec();
}

inline AIResponse runAnalyze(AIProvider &provider, const AIRequest &request, int timeoutMs = 15000)
{
    AIResponse result;
    awaitCallback(timeoutMs, [&](const std::function<void()> &done) {
        provider.analyze(request, [&result, done](AIResponse response) {
            result = std::move(response);
            done();
        });
    });
    return result;
}

}

#endif // ASYNC_TEST_SUPPORT_H

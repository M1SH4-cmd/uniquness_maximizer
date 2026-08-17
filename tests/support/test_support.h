#ifndef TEST_SUPPORT_H
#define TEST_SUPPORT_H

#include <QCoreApplication>
#include <QString>
#include <QTextStream>

#include <functional>
#include <initializer_list>

namespace TestSupport {

inline bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

inline int fail(const QString &message)
{
    QTextStream(stderr) << message << Qt::endl;
    return 1;
}

inline int runAll(std::initializer_list<std::function<bool()>> tests)
{
    for (const std::function<bool()> &test : tests) {
        if (!test()) return 1;
    }
    return 0;
}

}

#define TEST_SUPPORT_MAIN(...)                        \
    int main(int argc, char *argv[])                  \
    {                                                 \
        QCoreApplication application(argc, argv);      \
        Q_UNUSED(application)                         \
        return TestSupport::runAll({__VA_ARGS__});    \
    }

#endif // TEST_SUPPORT_H

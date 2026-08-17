#ifndef JSON_UTILS_H
#define JSON_UTILS_H

#include <QByteArray>
#include <QJsonObject>
#include <QString>

namespace JsonUtils {

bool parseObject(const QByteArray &json, QJsonObject &object, QString *errorMessage = nullptr);
bool parseObject(const QString &json, QJsonObject &object, QString *errorMessage = nullptr);
QByteArray toCompactJson(const QJsonObject &object);

}

#endif // JSON_UTILS_H

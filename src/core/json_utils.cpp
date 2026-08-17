#include "core/json_utils.h"

#include <QJsonDocument>
#include <QJsonParseError>

namespace JsonUtils {

bool parseObject(const QByteArray &json, QJsonObject &object, QString *errorMessage)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        if (errorMessage) *errorMessage = parseError.errorString();
        return false;
    }
    if (!document.isObject()) {
        if (errorMessage) *errorMessage = QStringLiteral("корневой элемент не является объектом");
        return false;
    }
    object = document.object();
    return true;
}

bool parseObject(const QString &json, QJsonObject &object, QString *errorMessage)
{
    return parseObject(json.toUtf8(), object, errorMessage);
}

QByteArray toCompactJson(const QJsonObject &object)
{
    return QJsonDocument(object).toJson(QJsonDocument::Compact);
}

}

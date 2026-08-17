#include "mainwindow.h"

#include <QApplication>
#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFrame>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QTextEdit>
#include <QTextStream>

namespace {
bool require(bool condition, const QString &message)
{
    if (condition) return true;
    QTextStream(stderr) << message << Qt::endl;
    return false;
}

QString keysDirectoryPath()
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("keys"));
}

void clearKeysDirectory()
{
    QDir(keysDirectoryPath()).removeRecursively();
}

bool writeKeyFile(const QString &fileName, const QByteArray &content)
{
    QDir().mkpath(keysDirectoryPath());
    QFile file(QDir(keysDirectoryPath()).filePath(fileName));
    if (!file.open(QIODevice::WriteOnly)) return false;
    return file.write(content) == content.size();
}

bool writeKey(const QString &id, const QString &provider, const QString &key, bool selected)
{
    const QJsonObject object{{QStringLiteral("id"), id}, {QStringLiteral("provider"), provider},
                             {QStringLiteral("key"), key}, {QStringLiteral("selected"), selected}};
    return writeKeyFile(id + QStringLiteral(".json"), QJsonDocument(object).toJson(QJsonDocument::Compact));
}

QJsonObject readKey(const QString &id)
{
    QFile file(QDir(keysDirectoryPath()).filePath(id + QStringLiteral(".json")));
    if (!file.open(QIODevice::ReadOnly)) return {};
    return QJsonDocument::fromJson(file.readAll()).object();
}

QComboBox *modelComboBox(MainWindow &window)
{
    return window.findChild<QComboBox *>(QStringLiteral("modelComboBox"));
}

QPushButton *reviewButton(MainWindow &window)
{
    return window.findChild<QPushButton *>(QStringLiteral("primaryButton"));
}

bool hasLabelContaining(MainWindow &window, const QString &text)
{
    const QList<QLabel *> labels = window.findChildren<QLabel *>();
    for (const QLabel *label : labels) {
        if (label->text().contains(text)) return true;
    }
    return false;
}

bool testStartsWithoutKeys()
{
    clearKeysDirectory();
    MainWindow window;
    QComboBox *combo = modelComboBox(window);
    QPushButton *review = reviewButton(window);
    if (!require(combo && review, QStringLiteral("no keys: не найдены элементы интерфейса"))) return false;
    return require(!combo->isEnabled() && combo->count() == 1, QStringLiteral("no keys: список моделей должен быть отключён"))
           && require(combo->itemData(0).isNull(), QStringLiteral("no keys: заглушка не должна содержать id ключа"))
           && require(!review->isEnabled(), QStringLiteral("no keys: проверка доступна без ключа"))
           && require(window.findChild<QFrame *>(QStringLiteral("requirementsPanel")) != nullptr,
                      QStringLiteral("no keys: панель требований должна быть в состоянии 'не готово'"));
}

bool testLoadsKeysAndSelectsActive()
{
    clearKeysDirectory();
    if (!require(writeKey(QStringLiteral("aaa"), QStringLiteral("OpenAI"), QStringLiteral("sk-first-1234"), false)
                     && writeKey(QStringLiteral("bbb"), QStringLiteral("Anthropic"), QStringLiteral("sk-second-5678"), true),
                 QStringLiteral("load: не удалось подготовить ключи"))) return false;

    MainWindow window;
    QComboBox *combo = modelComboBox(window);
    if (!require(combo != nullptr, QStringLiteral("load: не найден список моделей"))) return false;
    return require(combo->isEnabled() && combo->count() == 2, QStringLiteral("load: ключи не попали в список моделей"))
           && require(combo->itemData(0).toString() == QStringLiteral("aaa") && combo->itemData(1).toString() == QStringLiteral("bbb"),
                      QStringLiteral("load: порядок или id ключей неверны"))
           && require(combo->currentIndex() == 1, QStringLiteral("load: выбранный ключ не активирован в списке"))
           && require(hasLabelContaining(window, QStringLiteral("•••• 5678")), QStringLiteral("load: активный ключ не показан в маскированном виде"))
           && require(!hasLabelContaining(window, QStringLiteral("sk-second-5678")), QStringLiteral("load: ключ показан открытым текстом"));
}

bool testIgnoresInvalidKeyFiles()
{
    clearKeysDirectory();
    const bool prepared = writeKey(QStringLiteral("valid"), QStringLiteral("OpenAI"), QStringLiteral("sk-valid-9999"), true)
                          && writeKeyFile(QStringLiteral("broken.json"), QByteArrayLiteral("definitely not json"))
                          && writeKeyFile(QStringLiteral("nokey.json"), QByteArrayLiteral(R"({"id":"nokey","provider":"OpenAI"})"))
                          && writeKeyFile(QStringLiteral("noid.json"), QByteArrayLiteral(R"({"key":"sk-orphan","provider":"OpenAI"})"))
                          && writeKeyFile(QStringLiteral("ignored.txt"), QByteArrayLiteral(R"({"id":"txt","key":"sk-txt"})"));
    if (!require(prepared, QStringLiteral("invalid: не удалось подготовить файлы ключей"))) return false;

    MainWindow window;
    QComboBox *combo = modelComboBox(window);
    if (!require(combo != nullptr, QStringLiteral("invalid: не найден список моделей"))) return false;
    return require(combo->count() == 1 && combo->itemData(0).toString() == QStringLiteral("valid"),
                   QStringLiteral("invalid: повреждённые файлы ключей не отброшены"));
}

bool testSelectingKeyPersistsSelection()
{
    clearKeysDirectory();
    if (!require(writeKey(QStringLiteral("aaa"), QStringLiteral("OpenAI"), QStringLiteral("sk-first-1234"), false)
                     && writeKey(QStringLiteral("bbb"), QStringLiteral("Anthropic"), QStringLiteral("sk-second-5678"), true),
                 QStringLiteral("select: не удалось подготовить ключи"))) return false;

    MainWindow window;
    QComboBox *combo = modelComboBox(window);
    if (!require(combo != nullptr, QStringLiteral("select: не найден список моделей"))) return false;
    combo->setCurrentIndex(0);

    if (!require(readKey(QStringLiteral("aaa")).value(QStringLiteral("selected")).toBool(),
                 QStringLiteral("select: новый ключ не отмечен выбранным на диске"))) return false;
    if (!require(!readKey(QStringLiteral("bbb")).value(QStringLiteral("selected")).toBool(),
                 QStringLiteral("select: предыдущий ключ остался выбранным на диске"))) return false;
    if (!require(readKey(QStringLiteral("aaa")).value(QStringLiteral("key")).toString() == QStringLiteral("sk-first-1234"),
                 QStringLiteral("select: содержимое ключа искажено при сохранении"))) return false;

    MainWindow restarted;
    QComboBox *restartedCombo = modelComboBox(restarted);
    return require(restartedCombo && restartedCombo->currentIndex() == 0,
                   QStringLiteral("select: выбор не восстановлен после перезапуска"))
           && require(hasLabelContaining(restarted, QStringLiteral("•••• 1234")), QStringLiteral("select: активный ключ показан неверно"));
}

bool testDocumentAndKeyEnableReview()
{
    clearKeysDirectory();
    if (!require(writeKey(QStringLiteral("aaa"), QStringLiteral("OpenAI"), QStringLiteral("sk-first-1234"), true),
                 QStringLiteral("review: не удалось подготовить ключ"))) return false;

    MainWindow window(nullptr, "C:/work/diploma.docx");
    QPushButton *review = reviewButton(window);
    if (!require(review != nullptr, QStringLiteral("review: не найдена кнопка проверки"))) return false;
    if (!require(review->isEnabled(), QStringLiteral("review: проверка недоступна при готовом документе и ключе"))) return false;
    if (!require(window.findChild<QFrame *>(QStringLiteral("requirementsReady")) != nullptr,
                 QStringLiteral("review: панель требований не перешла в состояние 'готово'"))) return false;
    if (!require(hasLabelContaining(window, QStringLiteral("diploma.docx")), QStringLiteral("review: имя документа не показано"))) return false;

    review->click();
    QProgressBar *progress = window.findChild<QProgressBar *>(QStringLiteral("progress"));
    QTextEdit *preview = window.findChild<QTextEdit *>(QStringLiteral("preview"));
    return require(progress && progress->value() == 100, QStringLiteral("review: прогресс не завершён"))
           && require(preview && preview->toPlainText().contains(QStringLiteral("Проверка завершена")),
                      QStringLiteral("review: итоговый текст не показан"));
}
}

int main(int argc, char *argv[])
{
    if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("offscreen"));
    QApplication application(argc, argv);
    const bool passed = testStartsWithoutKeys() && testLoadsKeysAndSelectsActive() && testIgnoresInvalidKeyFiles()
                        && testSelectingKeyPersistsSelection() && testDocumentAndKeyEnableReview();
    clearKeysDirectory();
    return passed ? 0 : 1;
}

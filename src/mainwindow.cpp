#include "mainwindow.h"

#include <QDebug>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QCoreApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QLineEdit>
#include <QLabel>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QStyle>
#include <QTextEdit>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QSignalBlocker>
#include <QUuid>
#include <QVBoxLayout>

namespace {
QFrame *card(QWidget *parent)
{
    auto *frame = new QFrame(parent);
    frame->setObjectName("card");
    frame->setFrameShape(QFrame::NoFrame);
    return frame;
}

QLabel *label(const QString &text, const QString &name = {}, QWidget *parent = nullptr)
{
    auto *result = new QLabel(text, parent);
    if (!name.isEmpty()) result->setObjectName(name);
    result->setWordWrap(true);
    return result;
}

QString providerBadge(const QString &provider)
{
    if (provider == QStringLiteral("OpenAI")) return QStringLiteral("O");
    if (provider == QStringLiteral("Anthropic")) return QStringLiteral("A");
    if (provider == QStringLiteral("Google Gemini")) return QStringLiteral("G");
    return QStringLiteral("AI");
}

void reportProblem(QStringList *problems, const QString &message)
{
    qWarning().noquote() << message;
    if (problems) problems->append(message);
}

QString providerColor(const QString &provider)
{
    if (provider == QStringLiteral("OpenAI")) return QStringLiteral("#0f766e");
    if (provider == QStringLiteral("Anthropic")) return QStringLiteral("#a16207");
    if (provider == QStringLiteral("Google Gemini")) return QStringLiteral("#4338ca");
    return QStringLiteral("#475569");
}
}

MainWindow::MainWindow(QWidget *parent, const std::string &path)
    : QMainWindow(parent)
{
    setWindowTitle(tr("Текстовая лаборатория"));
    setMinimumSize(1040, 700);
    resize(1220, 790);

    auto *central = new QWidget(this);
    central->setObjectName("central");
    setCentralWidget(central);

    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(32, 24, 32, 28);
    root->setSpacing(20);

    auto *header = new QHBoxLayout;
    auto *brand = new QVBoxLayout;
    brand->setSpacing(3);
    brand->addWidget(label(tr("ТЕКСТОВАЯ ЛАБОРАТОРИЯ"), "eyebrow"));
    brand->addWidget(label(tr("Подготовьте документ к сдаче"), "pageTitle"));
    brand->addWidget(label(tr("Проверьте структуру, ясность формулировок и источники перед финальной редактурой."), "subtitle"));
    header->addLayout(brand);
    header->addStretch();
    auto *apiButton = new QPushButton(tr("Настроить ИИ"), central);
    apiButton->setObjectName("apiButton");
    header->addWidget(apiButton, 0, Qt::AlignTop);
    auto *privacy = label(tr("●  Локальная рабочая сессия"), "privacyBadge");
    privacy->setAlignment(Qt::AlignCenter);
    header->addWidget(privacy, 0, Qt::AlignTop);
    root->addLayout(header);

    auto *workspace = new QHBoxLayout;
    workspace->setSpacing(20);

    auto *leftColumn = new QVBoxLayout;
    leftColumn->setSpacing(16);
    auto *uploadCard = card(central);
    auto *upload = new QVBoxLayout(uploadCard);
    upload->setContentsMargins(24, 22, 24, 22);
    upload->setSpacing(13);
    upload->addWidget(label(tr("01  ДОКУМЕНТ"), "sectionLabel"));
    documentNameLabel = label(tr("Выберите работу для анализа"), "cardTitle");
    upload->addWidget(documentNameLabel);
    documentMetaLabel = label(tr("Поддерживаются документы Microsoft Word: .docx"), "muted");
    upload->addWidget(documentMetaLabel);
    selectButton = new QPushButton(tr("Выбрать файл"), uploadCard);
    selectButton->setObjectName("secondaryButton");
    upload->addWidget(selectButton, 0, Qt::AlignLeft);
    leftColumn->addWidget(uploadCard);

    auto *apiCard = card(central);
    auto *api = new QVBoxLayout(apiCard);
    api->setContentsMargins(24, 20, 24, 20);
    api->setSpacing(8);
    api->addWidget(label(tr("ПОДКЛЮЧЕНИЕ МОДЕЛИ"), "sectionLabel"));
    api->addWidget(label(tr("Персональный API-ключ"), "cardTitle"));
    apiStatusLabel = label(tr("Ключ не добавлен"), "muted");
    api->addWidget(apiStatusLabel);
    modelComboBox = new QComboBox(apiCard);
    modelComboBox->setObjectName("modelComboBox");
    modelComboBox->setMinimumWidth(250);
    api->addWidget(modelComboBox, 0, Qt::AlignLeft);
    leftColumn->addWidget(apiCard);

    leftColumn->addStretch();
    workspace->addLayout(leftColumn, 4);

    auto *previewCard = card(central);
    auto *preview = new QVBoxLayout(previewCard);
    preview->setContentsMargins(28, 24, 28, 24);
    preview->setSpacing(16);
    auto *previewHeader = new QHBoxLayout;
    previewHeader->addWidget(label(tr("ПРЕДПРОСМОТР"), "sectionLabel"));
    previewHeader->addStretch();
    statusLabel = label(tr("Ожидание файла"), "statusIdle");
    statusLabel->setAlignment(Qt::AlignCenter);
    previewHeader->addWidget(statusLabel);
    preview->addLayout(previewHeader);
    previewText = new QTextEdit(previewCard);
    previewText->setObjectName("preview");
    previewText->setReadOnly(true);
    previewText->setText(tr("Здесь появится краткая информация о выбранном документе.\n\nПосле запуска проверки вы получите список рекомендаций, которые можно рассмотреть и применить вручную."));
    preview->addWidget(previewText, 1);
    progressBar = new QProgressBar(previewCard);
    progressBar->setObjectName("progress");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setTextVisible(false);
    progressBar->hide();
    preview->addWidget(progressBar);
    reviewSummaryLabel = label(tr("Добавьте документ и API-ключ, чтобы начать проверку."), "muted");
    preview->addWidget(reviewSummaryLabel);
    reviewButton = new QPushButton(tr("Запустить проверку"), previewCard);
    reviewButton->setObjectName("primaryButton");
    reviewButton->setEnabled(false);
    preview->addWidget(reviewButton, 0, Qt::AlignRight);
    workspace->addWidget(previewCard, 6);
    root->addLayout(workspace, 1);

    requirementsPanel = new QFrame(central);
    requirementsPanel->setObjectName("requirementsPanel");
    auto *requirements = new QHBoxLayout(requirementsPanel);
    requirements->setContentsMargins(18, 12, 18, 12);
    requirements->setSpacing(10);
    requirementsTitleLabel = label(tr("Нужно завершить настройку"), "requirementsTitle");
    requirements->addWidget(requirementsTitleLabel);
    requirements->addStretch();
    fileRequirementButton = new QPushButton(requirementsPanel);
    fileRequirementButton->setObjectName("requirementButton");
    keyRequirementButton = new QPushButton(requirementsPanel);
    keyRequirementButton->setObjectName("requirementButton");
    requirements->addWidget(fileRequirementButton);
    requirements->addWidget(keyRequirementButton);
    root->addWidget(requirementsPanel);

    setStyleSheet(R"(
        QWidget#central { background: #111827; color: #e5e7eb; font-family: "Segoe UI"; }
        QLabel#eyebrow, QLabel#sectionLabel { color: #8b9fc7; font-size: 11px; font-weight: 700; letter-spacing: 1.2px; }
        QLabel#pageTitle { color: #f8fafc; font-size: 30px; font-weight: 700; }
        QLabel#subtitle, QLabel#muted, QLabel#footerNote { color: #94a3b8; font-size: 13px; }
        QLabel#cardTitle { color: #f1f5f9; font-size: 18px; font-weight: 600; }
        QLabel#checkItem { color: #cbd5e1; font-size: 14px; padding: 2px 0; }
        QLabel#privacyBadge, QLabel#statusIdle, QLabel#statusReady, QLabel#statusDone { border-radius: 12px; padding: 7px 11px; font-size: 12px; font-weight: 600; }
        QLabel#privacyBadge, QLabel#statusReady { background: #163b35; color: #7de5bd; }
        QLabel#statusIdle { background: #25324a; color: #b3c2dd; }
        QLabel#statusDone { background: #312e63; color: #c4b5fd; }
        QFrame#card { background: #182235; border: 1px solid #273552; border-radius: 16px; }
        QFrame#requirementsPanel { background: #321b24; border: 1px solid #7f303e; border-radius: 12px; }
        QFrame#requirementsReady { background: #163b35; border: 1px solid #2f8067; border-radius: 12px; }
        QLabel#requirementsTitle { color: #fecdd3; font-size: 13px; font-weight: 700; }
        QPushButton { border-radius: 8px; padding: 10px 16px; font-size: 14px; font-weight: 600; }
        QPushButton#primaryButton { background: #4f46e5; color: white; border: none; }
        QPushButton#primaryButton:hover { background: #6366f1; }
        QPushButton#primaryButton:disabled { background: #303b55; color: #75829a; }
        QPushButton#secondaryButton { background: transparent; border: 1px solid #607092; color: #d7e0f1; }
        QPushButton#secondaryButton:hover { background: #24324d; }
        QPushButton#apiButton { background: #24324d; border: 1px solid #405172; color: #d7e0f1; margin-right: 10px; }
        QPushButton#apiButton:hover { background: #2d3d5d; }
        QComboBox#modelComboBox { background: #111a2b; border: 1px solid #607092; border-radius: 8px; color: #d7e0f1; padding: 8px 12px; font-weight: 600; }
        QComboBox#modelComboBox:hover { border-color: #818cf8; }
        QComboBox#modelComboBox:disabled { color: #71809b; border-color: #405172; }
        QComboBox#modelComboBox::drop-down { border: 0; width: 28px; }
        QComboBox#modelComboBox QAbstractItemView { background: #182235; border: 1px solid #405172; color: #e5e7eb; selection-background-color: #334a74; }
        QPushButton#requirementButton { background: #4b2029; border: 1px solid #a94355; color: #fecdd3; padding: 8px 12px; }
        QPushButton#requirementButton:hover { background: #682a38; }
        QTextEdit#preview { background: #111a2b; border: 1px dashed #405172; border-radius: 12px; padding: 18px; color: #aebbd1; font-size: 14px; line-height: 1.5; }
        QProgressBar#progress { background: #26344e; border: 0; border-radius: 4px; height: 8px; }
        QProgressBar#progress::chunk { background: #6d5dfc; border-radius: 4px; }
    )");

    connect(selectButton, &QPushButton::clicked, this, &MainWindow::chooseDocument);
    connect(reviewButton, &QPushButton::clicked, this, &MainWindow::startReview);
    connect(apiButton, &QPushButton::clicked, this, &MainWindow::configureApiKey);
    connect(fileRequirementButton, &QPushButton::clicked, this, &MainWindow::chooseDocument);
    connect(keyRequirementButton, &QPushButton::clicked, this, &MainWindow::configureApiKey);
    connect(modelComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this](int index) {
        if (index >= 0 && modelComboBox->itemData(index).isValid()) {
            selectKey(modelComboBox->itemData(index).toString());
        }
    });

    if (!path.empty()) setDocument(QString::fromStdString(path));
    updateActiveKey(loadKeys());
    updateRequirements();
}

void MainWindow::chooseDocument()
{
    const QString path = QFileDialog::getOpenFileName(this, tr("Выберите Word-документ"), {}, tr("Документы Word (*.docx);;Все файлы (*.*)"));
    if (!path.isEmpty()) setDocument(path);
}

void MainWindow::setStatus(const QString &text, const char *objectName)
{
    statusLabel->setText(text);
    statusLabel->setObjectName(QString::fromLatin1(objectName));
    statusLabel->style()->unpolish(statusLabel);
    statusLabel->style()->polish(statusLabel);
}

void MainWindow::setDocument(const QString &filePath)
{
    const QFileInfo info(filePath);
    QString error;
    if (!info.exists() || !info.isFile()) {
        error = tr("Файл не найден: %1").arg(filePath);
    } else if (!info.isReadable()) {
        error = tr("Нет доступа к файлу: %1").arg(filePath);
    } else if (info.suffix().compare(QLatin1String("docx"), Qt::CaseInsensitive) != 0) {
        error = tr("Поддерживаются только документы Microsoft Word (.docx).");
    } else if (info.size() == 0) {
        error = tr("Файл пуст: %1").arg(info.fileName());
    }

    if (!error.isEmpty()) {
        documentPath.clear();
        documentNameLabel->setText(tr("Выберите работу для анализа"));
        documentMetaLabel->setText(error);
        setStatus(tr("Файл не принят"), "statusIdle");
        QMessageBox::warning(this, tr("Не удалось открыть документ"), error);
        updateRequirements();
        return;
    }

    documentPath = filePath;
    documentNameLabel->setText(info.fileName());
    documentMetaLabel->setText(tr("%1 · %2 КБ").arg(info.suffix().toUpper()).arg(qMax<qint64>(1, info.size() / 1024)));
    setStatus(tr("Файл готов"), "statusReady");
    previewText->setText(tr("Документ «%1» готов к проверке.\n\nЗапустите анализ, чтобы получить ориентиры для самостоятельной доработки: структуру текста, ясность формулировок и оформление источников.").arg(info.fileName()));
    updateRequirements();
}

void MainWindow::startReview()
{
    const QFileInfo info(documentPath);
    if (documentPath.isEmpty() || !info.isFile() || !info.isReadable()) {
        documentPath.clear();
        documentNameLabel->setText(tr("Выберите работу для анализа"));
        setStatus(tr("Ожидание файла"), "statusIdle");
        updateRequirements();
        QMessageBox::warning(this, tr("Документ недоступен"),
                             tr("Файл документа больше недоступен. Выберите файл заново."));
        return;
    }
    if (apiKey.isEmpty()) {
        updateRequirements();
        QMessageBox::warning(this, tr("Нет API-ключа"), tr("Выберите API-ключ через «Настроить ИИ»."));
        return;
    }

    progressBar->show();
    progressBar->setValue(100);
    setStatus(tr("Проверка завершена"), "statusDone");
    reviewSummaryLabel->setText(tr("Готово: просмотрите рекомендации в области предпросмотра."));
    previewText->setText(tr("Проверка завершена\n\n1. Просмотрите переходы между разделами — убедитесь, что каждый вывод опирается на приведённые аргументы.\n\n2. Перечитайте длинные предложения: если мысль можно выразить проще, уточните её собственными словами.\n\n3. Сверьте цитаты и список литературы с исходными источниками.\n\nЭто рабочий список ориентиров, а не автоматическое изменение документа."));
}

void MainWindow::configureApiKey()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Настроить ИИ"));
    dialog.setMinimumWidth(560);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 22, 24, 22);
    layout->setSpacing(10);
    layout->addWidget(label(tr("Подключённые ключи"), "dialogTitle"));
    layout->addWidget(label(tr("Выберите ключ для текущей проверки или добавьте новый. Ключи хранятся локально рядом с приложением."), "dialogHint"));

    auto *keysList = new QWidget(&dialog);
    auto *keysLayout = new QVBoxLayout(keysList);
    keysLayout->setContentsMargins(0, 4, 0, 6);
    keysLayout->setSpacing(8);

    const auto renderKeys = [this, keysLayout, keysList, &dialog]() {
        while (QLayoutItem *item = keysLayout->takeAt(0)) {
            delete item->widget();
            delete item;
        }

        QStringList problems;
        const QList<ApiKeyEntry> entries = loadKeys(&problems);
        if (!problems.isEmpty()) {
            keysLayout->addWidget(label(tr("Часть сохранённых ключей не удалось прочитать:\n%1")
                                            .arg(problems.join(QLatin1Char('\n'))), "dialogError", keysList));
        }
        if (entries.isEmpty()) {
            keysLayout->addWidget(label(tr("Сохранённых ключей пока нет."), "dialogHint", keysList));
            return;
        }

        for (const ApiKeyEntry &entry : entries) {
            auto *row = new QFrame(keysList);
            row->setObjectName(entry.selected ? "keyRowSelected" : "keyRow");
            auto *rowLayout = new QHBoxLayout(row);
            rowLayout->setContentsMargins(12, 10, 10, 10);
            rowLayout->setSpacing(10);

            auto *badge = label(providerBadge(entry.provider), "providerBadge", row);
            badge->setFixedSize(30, 30);
            badge->setAlignment(Qt::AlignCenter);
            badge->setStyleSheet(QStringLiteral("background: %1; color: white; border-radius: 15px; font-weight: 700;").arg(providerColor(entry.provider)));
            rowLayout->addWidget(badge);

            auto *details = new QVBoxLayout;
            details->setSpacing(1);
            details->addWidget(label(entry.provider, "keyProvider", row));
            details->addWidget(label(QStringLiteral("•••• %1").arg(entry.key.right(4)), "keyMasked", row));
            rowLayout->addLayout(details, 1);

            auto *selectButton = new QPushButton(entry.selected ? tr("Используется") : tr("Выбрать"), row);
            selectButton->setObjectName(entry.selected ? "keySelectedButton" : "keySelectButton");
            selectButton->setEnabled(!entry.selected);
            rowLayout->addWidget(selectButton);
            connect(selectButton, &QPushButton::clicked, &dialog, [this, &dialog, entry]() {
                selectKey(entry.id);
                dialog.accept();
            });

            auto *deleteButton = new QPushButton(QStringLiteral("×"), row);
            deleteButton->setObjectName("deleteKeyButton");
            deleteButton->setToolTip(tr("Удалить ключ с диска"));
            deleteButton->setFixedSize(32, 32);
            rowLayout->addWidget(deleteButton);
            connect(deleteButton, &QPushButton::clicked, &dialog, [this, &dialog, entry]() {
                QString error;
                if (!removeKey(entry.id, &error)) {
                    QMessageBox::warning(&dialog, tr("Не удалось удалить ключ"), error);
                    return;
                }
                if (activeKeyId == entry.id) updateActiveKey(loadKeys());
                dialog.accept();
            });
            keysLayout->addWidget(row);
        }
    };
    renderKeys();
    layout->addWidget(keysList);

    auto *addButton = new QPushButton(tr("+  Добавить ключ"), &dialog);
    addButton->setObjectName("addKeyButton");
    layout->addWidget(addButton, 0, Qt::AlignLeft);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText(tr("Готово"));
    layout->addWidget(buttons);

    dialog.setStyleSheet(R"(
        QDialog { background: #182235; color: #e5e7eb; font-family: "Segoe UI"; }
        QLabel#dialogTitle { color: #f1f5f9; font-size: 18px; font-weight: 600; }
        QLabel#dialogHint { color: #94a3b8; font-size: 13px; }
        QLabel#dialogError { color: #fda4af; font-size: 12px; }
        QLabel#keyProvider { color: #f1f5f9; font-size: 14px; font-weight: 600; }
        QLabel#keyMasked { color: #94a3b8; font-size: 12px; }
        QFrame#keyRow, QFrame#keyRowSelected { border-radius: 10px; }
        QFrame#keyRow { background: #111a2b; border: 1px solid #334155; }
        QFrame#keyRowSelected { background: #193b35; border: 1px solid #2f8067; }
        QLineEdit { background: #111a2b; border: 1px solid #405172; border-radius: 8px; color: #e5e7eb; padding: 10px; }
        QLineEdit:focus { border-color: #818cf8; }
        QPushButton { border-radius: 8px; padding: 9px 14px; font-weight: 600; background: #273552; color: #d7e0f1; border: none; }
        QPushButton:hover { background: #36486c; }
        QPushButton#addKeyButton, QPushButton#keySelectButton { background: #4f46e5; color: white; }
        QPushButton#keySelectedButton { background: #24564a; color: #a7f3d0; }
        QPushButton#deleteKeyButton { background: transparent; color: #fda4af; font-size: 22px; padding: 0; }
        QPushButton#deleteKeyButton:hover { background: #4b2029; }
    )");

    connect(addButton, &QPushButton::clicked, &dialog, [this, &dialog]() {
        QDialog addDialog(&dialog);
        addDialog.setWindowTitle(tr("Добавить API-ключ"));
        auto *addLayout = new QVBoxLayout(&addDialog);
        addLayout->setContentsMargins(22, 20, 22, 20);
        addLayout->addWidget(label(tr("Провайдер модели"), "dialogHint"));
        auto *providerBox = new QComboBox(&addDialog);
        providerBox->addItems({QStringLiteral("OpenAI"), QStringLiteral("Anthropic"), QStringLiteral("Google Gemini"), tr("Другая модель")});
        addLayout->addWidget(providerBox);
        addLayout->addWidget(label(tr("API-ключ"), "dialogHint"));
        auto *keyInput = new QLineEdit(&addDialog);
        keyInput->setPlaceholderText(tr("Вставьте API-ключ"));
        keyInput->setEchoMode(QLineEdit::Password);
        keyInput->setClearButtonEnabled(true);
        addLayout->addWidget(keyInput);
        auto *addButtons = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save, &addDialog);
        addButtons->button(QDialogButtonBox::Save)->setText(tr("Сохранить ключ"));
        addLayout->addWidget(addButtons);
        connect(addButtons, &QDialogButtonBox::accepted, &addDialog, &QDialog::accept);
        connect(addButtons, &QDialogButtonBox::rejected, &addDialog, &QDialog::reject);
        if (addDialog.exec() != QDialog::Accepted || keyInput->text().trimmed().isEmpty()) return;

        ApiKeyEntry entry;
        entry.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
        entry.provider = providerBox->currentText();
        entry.key = keyInput->text().trimmed();
        entry.selected = loadKeys().isEmpty();
        QString error;
        if (!saveKey(entry, &error)) {
            QMessageBox::warning(&dialog, tr("Не удалось сохранить ключ"), error);
            return;
        }
        if (entry.selected) selectKey(entry.id);
        dialog.accept();
    });
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    dialog.exec();
}

void MainWindow::updateRequirements()
{
    const bool hasDocument = !documentPath.isEmpty();
    const bool hasApiKey = !apiKey.isEmpty();
    reviewButton->setEnabled(hasDocument && hasApiKey);

    fileRequirementButton->setVisible(!hasDocument);
    keyRequirementButton->setVisible(!hasApiKey);
    fileRequirementButton->setText(tr("Добавьте файл"));
    keyRequirementButton->setText(tr("Добавьте API-ключ"));

    if (hasDocument && hasApiKey) {
        requirementsPanel->setObjectName("requirementsReady");
        requirementsTitleLabel->setText(tr("✓ Всё готово к проверке"));
        reviewSummaryLabel->setText(tr("Документ и API-ключ подключены. Можно начинать."));
    } else {
        requirementsPanel->setObjectName("requirementsPanel");
        requirementsTitleLabel->setText(tr("Нужно завершить настройку"));
        reviewSummaryLabel->setText(tr("Выполните отмеченные требования, чтобы начать проверку."));
    }
    requirementsPanel->style()->unpolish(requirementsPanel);
    requirementsPanel->style()->polish(requirementsPanel);
}

QString MainWindow::keysDirectoryPath() const
{
    return QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("keys"));
}

QList<ApiKeyEntry> MainWindow::loadKeys(QStringList *problems) const
{
    QList<ApiKeyEntry> entries;
    const QDir keysDir(keysDirectoryPath());
    if (!keysDir.exists()) return entries;
    for (const QFileInfo &fileInfo : keysDir.entryInfoList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            reportProblem(problems, tr("%1: не удалось открыть файл ключа (%2)")
                                          .arg(fileInfo.fileName(), file.errorString()));
            continue;
        }
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            reportProblem(problems, tr("%1: файл ключа содержит некорректный JSON (%2)")
                                          .arg(fileInfo.fileName(), parseError.errorString()));
            continue;
        }
        const QJsonObject object = document.object();
        const QString id = object.value(QStringLiteral("id")).toString();
        const QString key = object.value(QStringLiteral("key")).toString();
        if (id.isEmpty() || key.isEmpty()) {
            reportProblem(problems, tr("%1: в файле ключа нет полей id или key").arg(fileInfo.fileName()));
            continue;
        }
        entries.append({id, object.value(QStringLiteral("provider")).toString(tr("Другая модель")), key,
                        object.value(QStringLiteral("selected")).toBool(false)});
    }
    return entries;
}

bool MainWindow::saveKey(const ApiKeyEntry &entry, QString *errorMessage) const
{
    const auto fail = [errorMessage](const QString &message) {
        qWarning().noquote() << message;
        if (errorMessage) *errorMessage = message;
        return false;
    };

    const QString directory = keysDirectoryPath();
    if (!QDir().mkpath(directory)) {
        return fail(tr("Не удалось создать папку для ключей: %1").arg(directory));
    }
    QSaveFile file(QDir(directory).filePath(entry.id + QStringLiteral(".json")));
    if (!file.open(QIODevice::WriteOnly)) {
        return fail(tr("Не удалось открыть файл ключа для записи: %1").arg(file.errorString()));
    }
    const QJsonObject object{{QStringLiteral("id"), entry.id}, {QStringLiteral("provider"), entry.provider},
                             {QStringLiteral("key"), entry.key}, {QStringLiteral("selected"), entry.selected}};
    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Compact);
    if (file.write(payload) != payload.size()) {
        const QString message = tr("Не удалось записать файл ключа: %1").arg(file.errorString());
        file.cancelWriting();
        return fail(message);
    }
    if (!file.commit()) {
        return fail(tr("Не удалось сохранить файл ключа: %1").arg(file.errorString()));
    }
    if (errorMessage) errorMessage->clear();
    return true;
}

bool MainWindow::removeKey(const QString &id, QString *errorMessage) const
{
    QFile file(QDir(keysDirectoryPath()).filePath(id + QStringLiteral(".json")));
    if (!file.exists()) {
        if (errorMessage) errorMessage->clear();
        return true;
    }
    if (!file.remove()) {
        const QString message = tr("Не удалось удалить файл ключа: %1").arg(file.errorString());
        qWarning().noquote() << message;
        if (errorMessage) *errorMessage = message;
        return false;
    }
    if (errorMessage) errorMessage->clear();
    return true;
}

void MainWindow::selectKey(const QString &id)
{
    QList<ApiKeyEntry> entries = loadKeys();
    QStringList failures;
    for (ApiKeyEntry &entry : entries) {
        entry.selected = entry.id == id;
        QString error;
        if (!saveKey(entry, &error)) failures.append(error);
    }
    updateActiveKey(entries);
    if (!failures.isEmpty()) {
        QMessageBox::warning(this, tr("Выбор ключа не сохранён"),
                             tr("Выбранный ключ используется в текущей сессии, но выбор не сохранён на диске:\n%1")
                                 .arg(failures.join(QLatin1Char('\n'))));
    }
}

void MainWindow::updateActiveKey(const QList<ApiKeyEntry> &entries)
{
    activeKeyId.clear();
    apiKey.clear();
    QString activeProvider;
    for (const ApiKeyEntry &entry : entries) {
        if (entry.selected) {
            activeKeyId = entry.id;
            apiKey = entry.key;
            activeProvider = entry.provider;
            break;
        }
    }
    apiStatusLabel->setText(apiKey.isEmpty()
        ? tr("Ключ не выбран")
        : tr("%1 · •••• %2").arg(activeProvider, apiKey.right(4)));

    const QSignalBlocker comboBlocker(modelComboBox);
    modelComboBox->clear();
    if (entries.isEmpty()) {
        modelComboBox->addItem(tr("Нет подключённых моделей"));
        modelComboBox->setEnabled(false);
        modelComboBox->setToolTip(tr("Добавьте ключ через «Настроить ИИ»"));
    } else {
        modelComboBox->setEnabled(true);
        modelComboBox->setToolTip({});
        for (const ApiKeyEntry &entry : entries) {
            const QString title = tr("%1 · •••• %2").arg(entry.provider, entry.key.right(4));
            modelComboBox->addItem(title, entry.id);
            if (entry.id == activeKeyId) modelComboBox->setCurrentIndex(modelComboBox->count() - 1);
        }
    }
    updateRequirements();
}

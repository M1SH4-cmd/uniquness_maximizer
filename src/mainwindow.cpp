#include "mainwindow.h"
#include "maximizer.h"

MainWindow::MainWindow(QWidget *parent, std::string path)
    : QMainWindow(parent)
{

    Maximizer max(path);

    QWidget *centralWidget = new QWidget;
    QWidget *buffer = new QWidget;
    buffer->resize(340, 10);
    setCentralWidget(centralWidget);

    centralVLayout = new QVBoxLayout(centralWidget);

    if (!path.empty()) {
        argv1Path = QString::fromStdString(path);
    }

    documentPreviewLayout = new QHBoxLayout;
    indicatorsLayout = new QHBoxLayout;
    fileLaunchLayout = new QHBoxLayout;
    bufferForStuffLayout = new QHBoxLayout;

    currentUniqLayout = new QHBoxLayout;
    outUniqLayout = new QHBoxLayout;
    hasAiLayout = new QHBoxLayout;

    documentPreviewTxtE = new QTextEdit;

    documentPreviewLbl = new QLabel("Поле предпросмотра:");
    chooseFileLbl = new QLabel("Работа с файлом:");
    currentUniqLbl = new QLabel("Изначальная уникальность текста");
    currentUniqNumLbl = new QLabel("0");
    outUniqLbl = new QLabel("Полученная уникальность текста");
    outUniqNumLbl = new QLabel("0");
    hasAipresenceLbl = new QLabel("Присутствие ИИ:");
    isAI = new QLabel("Нет");

    chooseFileBtn = new QPushButton("Выберете файл");
    maxUniqBtn = new QPushButton("Начать работу");
    documentNameLnE = new QLineEdit;

    centralVLayout->addLayout(bufferForStuffLayout);
    bufferForStuffLayout->addWidget(documentPreviewLbl);
    bufferForStuffLayout->addWidget(documentNameLnE);

    centralVLayout->addLayout(documentPreviewLayout);
    documentPreviewLayout->addWidget(documentPreviewTxtE);

    centralVLayout->addLayout(indicatorsLayout);
    indicatorsLayout->addLayout(currentUniqLayout);
    currentUniqLayout->addWidget(currentUniqLbl);
    currentUniqLayout->addWidget(currentUniqNumLbl);

    indicatorsLayout->addLayout(outUniqLayout);
    outUniqLayout->addWidget(outUniqLbl);
    outUniqLayout->addWidget(outUniqNumLbl);

    indicatorsLayout->addLayout(hasAiLayout);
    hasAiLayout->addWidget(hasAipresenceLbl);
    hasAiLayout->addWidget(isAI);

    indicatorsLayout->addWidget(buffer);

    centralVLayout->addLayout(fileLaunchLayout);
    fileLaunchLayout->addWidget(chooseFileLbl);
    fileLaunchLayout->addWidget(chooseFileBtn);
    fileLaunchLayout->addWidget(maxUniqBtn);

    QObject::connect(chooseFileBtn, &QPushButton::clicked, this, [this]() {
        QString filePath = QFileDialog::getOpenFileName(
            this,
            tr("Выберите Word-файл"),
            QString(),
            tr("Word документы (*.doc *.docx)")
            );

        if (filePath.isEmpty())
            return;

        argv1Path = filePath;
        documentNameLnE->clear();
        documentNameLnE->setText(filePath);
    });

    QObject::connect(maxUniqBtn, &QPushButton::clicked, this, [](){});

}

MainWindow::~MainWindow() {}

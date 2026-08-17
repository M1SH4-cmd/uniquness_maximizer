#include "mainwindow.h"
#include <QApplication>
#include <QFileInfo>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    std::string path;
    if (argc > 1) {
        const QFileInfo fileInfo(QString::fromLocal8Bit(argv[1]));
        if (fileInfo.isFile() && fileInfo.suffix().compare(QLatin1String("docx"), Qt::CaseInsensitive) == 0) {
            path = fileInfo.absoluteFilePath().toStdString();
        }
    }
    MainWindow w(nullptr, path);
    a.setWindowIcon(QIcon(":/res/resources/icon.ico"));
    w.show();

    return a.exec();
}

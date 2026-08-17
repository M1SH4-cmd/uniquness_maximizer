#include "mainwindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    std::string path;
    if (argc > 1) {
        path = argv[1];
    }
    MainWindow w(nullptr, path);
    a.setWindowIcon(QIcon(":/res/resources/icon.ico"));
    w.show();

    return a.exec();
}

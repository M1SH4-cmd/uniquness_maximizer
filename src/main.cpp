#include "mainwindow.h"
#include <QApplication>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    a.setWindowIcon(QIcon(":/res/resources/icon.ico"));
    w.show();


    std::string path;

    if (argc > 1) {
        path = argv[1];
    }

    return a.exec();
}

#include "mainwindow.h"
#include <QApplication>
#include "pugixml.hpp"
#include <zip.h>

void testZip(const std::string& path) {
    zip* z = zip_open(path.c_str(), ZIP_RDONLY, nullptr);
    if (!z) {
        qDebug() << "libzip не работает";
    } else {
        qDebug() << "libzip OK";
        zip_close(z);
    }
}

int main(int argc, char *argv[])
{
    // Тест библиотеки pugixml, правильно ли я блять её подключил
    // pugi::xml_document doc;
    // doc.load_string("<root><a>test</a></root>");

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

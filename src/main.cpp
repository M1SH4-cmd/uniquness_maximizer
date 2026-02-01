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
    //MainWindow w;
    a.setWindowIcon(QIcon(":/res/resources/icon.ico"));
    //w.show();

    std::string path = "C:\\Main\\test_word.docx";
    testZip(path);

    if (argc > 1) {
        path = argv[1];
    }

    return a.exec();
}

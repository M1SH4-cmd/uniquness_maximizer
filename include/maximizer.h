#ifndef MAXIMIZER_H
#define MAXIMIZER_H

#include <thread>
#include <QDebug>
#include <QFile>
#include <QDir>
#include <fstream>

class Maximizer { // "Это класс оркестратор

public:
    Maximizer(std::string path);

private:
    std::string path;



};

#endif // MAXIMIZER_H

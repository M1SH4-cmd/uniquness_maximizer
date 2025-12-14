#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QFileDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QTextEdit>
#include <QLabel>
#include <QDir>
#include <QIcon>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr, std::string path = "");
    ~MainWindow();


private:
    QString argv1Path;

    QVBoxLayout *centralVLayout;
    QHBoxLayout *bufferForStuffLayout;
    QHBoxLayout *documentPreviewLayout;
    QHBoxLayout *indicatorsLayout;
    QHBoxLayout *fileLaunchLayout;

    QHBoxLayout *currentUniqLayout;
    QHBoxLayout *outUniqLayout;
    QHBoxLayout *hasAiLayout;


    QTextEdit *documentPreviewTxtE;

    QLabel *documentPreviewLbl;
    QLabel *chooseFileLbl;
    QLabel *currentUniqLbl;
    QLabel *currentUniqNumLbl;
    QLabel *outUniqLbl;
    QLabel *outUniqNumLbl;
    QLabel *hasAipresenceLbl;
    QLabel *isAI;

    QPushButton *chooseFileBtn;
    QPushButton *maxUniqBtn;

    QLineEdit *documentNameLnE;

};
#endif // MAINWINDOW_H

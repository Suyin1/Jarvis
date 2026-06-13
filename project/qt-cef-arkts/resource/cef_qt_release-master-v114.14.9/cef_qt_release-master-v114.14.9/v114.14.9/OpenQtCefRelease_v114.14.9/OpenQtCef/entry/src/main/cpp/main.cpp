#include <QDebug>
#include <QApplication>
#include "mainwindow/mainwindow.h"

int main(int argc, char *argv[])
{
    qInfo() << "Hello, Qt For OpenHarmony!";
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
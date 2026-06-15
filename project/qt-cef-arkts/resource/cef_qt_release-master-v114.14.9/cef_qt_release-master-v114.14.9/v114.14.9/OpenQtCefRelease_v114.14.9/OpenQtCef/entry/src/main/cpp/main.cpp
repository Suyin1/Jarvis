// ============================================================
// main.cpp —— CEF+QT+ArkTS 应用入口
// 功能：创建 QApplication 和 MainWindow（Qt 主窗口），
//       启动 Qt 事件循环。MainWindow 内部管理 CEF 初始化、
//       浏览器创建和 ArkUI 原生节点适配。
// ============================================================

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
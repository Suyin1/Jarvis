/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "testcef/TCSimpleHandler.h"
#include <QMainWindow>
#include <QObject>
#include "newwindow.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void chromiumMainthread();
    void InitControls();
    void OnChildWindowClosed(NewWindow* window); // 仅清空单个子窗口指针
    int InitCef(int argc, char *argv[]);
    bool CreateBrowser(const QString &url);
    void LoadURL(const QString &url);
    void OnJump();
    long GetWinId();
    int64_t GetBrowserId();
    long GetBrowserWindowHandle();
    void onBrowserCreated(int64_t identifier);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void closeAllBrowsers(QCloseEvent *event);
    void checkCefShutdown(QCloseEvent *event);
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    NewWindow* newWindow_ = nullptr; 
    bool browserCreated_ = false;
    QString default_url_ = "https://ams.haitaibrowser.com:8000/ohcef/ohindex.html";
    QString url_ = "";
    CefRefPtr<TCSimpleHandler> handler_ = nullptr;
    int64_t identifier_ = 0;
    bool shutdown_ = false;
    bool cefInstantiated_ = false;
};

#endif // MAINWINDOW_H
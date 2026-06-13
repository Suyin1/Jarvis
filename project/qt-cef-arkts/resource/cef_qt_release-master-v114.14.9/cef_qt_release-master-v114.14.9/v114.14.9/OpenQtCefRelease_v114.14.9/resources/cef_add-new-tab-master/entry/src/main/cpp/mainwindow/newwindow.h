/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#ifndef OPENQTCEF_NEWWINDOW_H
#define OPENQTCEF_NEWWINDOW_H

#include "testcef/TCSimpleHandler.h"
#include <QMainWindow>
#include <QObject>
#include <QTabWidget>
#include <QWidget>

struct TabInfo {
    int64_t browserId = 0;
    long windowHandle = 0; 
    bool browserCreated = false; 
    QWidget* pageWidget = nullptr; 
};

class NewWindow : public QMainWindow {
    Q_OBJECT

public:
    NewWindow(CefRefPtr<TCSimpleHandler> handler, QWidget *parent = nullptr);
    ~NewWindow();
    
    void addNewTab(const QString& url = "https://developer.huawei.com/consumer/cn/");
    void LoadURL(int tabIndex, const QString &url);

signals:
    void windowClosed(NewWindow* window);

protected:
    void resizeEvent(QResizeEvent *event) override;  
    void closeEvent(QCloseEvent *event) override;    

private slots:
    void onTabCloseRequested(int tabIndex);
    void onBrowserCreated(int tabIndex, int64_t identifier);

private:
    int getTabIndexByWidget(QWidget* pageWidget);
    bool createTabBrowser(int tabIndex, const QString &url);
    long GetTabWinId(int tabIndex);

    QTabWidget* tabWidget_ = nullptr; 
    QList<TabInfo> tabInfoList_;      
    CefRefPtr<TCSimpleHandler> handler_ = nullptr;
    QString defaultTabUrl_ = "file:///data/storage/el2/base/haps/entry/files/helloworld.html"; 
};

#endif //OPENQTCEF_NEWWINDOW_H
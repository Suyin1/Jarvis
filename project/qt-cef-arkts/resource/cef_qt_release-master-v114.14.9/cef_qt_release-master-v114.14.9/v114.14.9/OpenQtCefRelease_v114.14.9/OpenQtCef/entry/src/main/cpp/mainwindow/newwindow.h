//
// Created on 2025/7/16.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef OPENQTCEF_NEWWINDOW_H
#define OPENQTCEF_NEWWINDOW_H

#include "testcef/TCSimpleHandler.h"
#include <QMainWindow>
#include <QObject>
#include <QTabWidget>

class NewWindow : public QMainWindow {
    Q_OBJECT

public:
    NewWindow(CefRefPtr<TCSimpleHandler> handler, int index, QPoint pt, QSize sz, QWidget *parent = nullptr);
    ~NewWindow();

    bool CreateBrowser(const QString &url);
    void LoadURL(const QString &url);
    long GetWinId();
    int64_t GetBrowserId();
    long GetBrowserWindowHandle();
    void onBrowserCreated(int64_t identifier);

signals:
    void windowClosed(NewWindow* window);
    void browserReady(NewWindow* window, qint64 identifier);

protected:
    void resizeEvent(QResizeEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;
    void focusOutEvent(QFocusEvent *event) override;

private:
    bool browserCreated_ = false;
    QString url_ = "https://www.bing.com";
    CefRefPtr<TCSimpleHandler> handler_ = nullptr;
    // For browser id, Only one browser can be created in a window
    int64_t identifier_ = 0;
};


#endif //OPENQTCEF_NEWWINDOW_H

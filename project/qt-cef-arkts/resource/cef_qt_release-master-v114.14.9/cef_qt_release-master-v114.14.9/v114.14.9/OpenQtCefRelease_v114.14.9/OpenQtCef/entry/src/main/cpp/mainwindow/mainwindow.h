// ============================================================
// mainwindow.h —— Qt 主窗口声明
// 功能：管理 CEF 初始化、浏览器创建、子窗口管理、ArkUI 原生
//       节点适配操作。作为 CEF+QT+ArkTS 架构的核心协调者。
// ============================================================

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "testcef/TCSimpleHandler.h"

#include <QMainWindow>
#include <QObject>
#include "newwindow.h"
#include "testdialog.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void chromiumMainthread();           // 在独立线程中启动 CEF
    void initControls();                 // 初始化 UI 控件（地址栏、按钮等）
    void onChildWindowClosed(NewWindow* window);   // 子窗口关闭回调
    void onChildBrowserReady(NewWindow* window, qint64 identifier);  // 子浏览器就绪回调
    int initCef(int argc, char *argv[]); // 初始化 CEF 引擎
    bool CreateBrowser(const QString &url);  // 创建 CEF 浏览器实例
    void LoadURL(const QString &url);    // 加载 URL
    void onJump();                       // 地址栏跳转
    long GetWinId();                     // 获取 Qt 窗口原生句柄
    int64_t GetBrowserId();              // 获取浏览器 ID
    long GetBrowserWindowHandle();       // 获取浏览器窗口句柄
    void onBrowserCreated(int64_t identifier);  // 浏览器创建完成回调

protected:
    void resizeEvent(QResizeEvent *event) override;   // 窗口大小变化时调整 CEF 节点
    void closeEvent(QCloseEvent *event) override;     // 窗口关闭时优雅关闭 CEF
    void closeAllBrowsers(QCloseEvent *event);        // 关闭所有浏览器
    void checkCefShutdown(QCloseEvent *event);        // 检查 CEF 是否已关闭
    void focusInEvent(QFocusEvent *event) override;   // 焦点事件转发到 ArkUI 节点

    int currentCreatingIndex_ = 0;
    void createNextWindow(bool is_child);              // 顺序创建下一个测试窗口
    void createWindowsSequentially(bool is_child);     // 连续创建多个测试窗口

private:
    std::vector<NewWindow*> newWindows_;          // 子窗口列表
    TestDialog* testDialog_ = nullptr;            // 独立测试对话框
    TestDialog* testDialog2_ = nullptr;           // 子测试对话框
    bool browserCreated_ = false;                 // 浏览器是否已创建
    QString default_url_ = "https://ams.haitaibrowser.com:8000/ohcef/ohindex.html";
    QString url_ = "";
    CefRefPtr<TCSimpleHandler> handler_ = nullptr; // CEF 浏览器事件处理器
    int64_t identifier_ = 0;                      // 当前浏览器 ID
    bool shutdown_ = false;                       // CEF 是否已关闭
    bool cef_instantiated_ = false;               // CEF 是否已初始化
};

#endif // MAINWINDOW_H

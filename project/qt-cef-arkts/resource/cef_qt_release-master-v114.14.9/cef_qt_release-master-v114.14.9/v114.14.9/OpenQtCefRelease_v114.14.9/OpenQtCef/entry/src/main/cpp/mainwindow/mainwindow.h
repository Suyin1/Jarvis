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
    void chromiumMainthread();
    void initControls();
    void onChildWindowClosed(NewWindow* window);
    void onChildBrowserReady(NewWindow* window, qint64 identifier);
    int initCef(int argc, char *argv[]);
    bool CreateBrowser(const QString &url);
    void LoadURL(const QString &url);
    void onJump();
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

    int currentCreatingIndex_ = 0;
    void createNextWindow(bool is_child);
    void createWindowsSequentially(bool is_child);
    
private:
    std::vector<NewWindow*> newWindows_;
    TestDialog* testDialog_ = nullptr;
    TestDialog* testDialog2_ = nullptr;
    bool browserCreated_ = false;
    QString default_url_ = "https://ams.haitaibrowser.com:8000/ohcef/ohindex.html";
    QString url_ = "";
    CefRefPtr<TCSimpleHandler> handler_ = nullptr;
    // For browser id, Only one browser can be created in a window
    int64_t identifier_ = 0;
    bool shutdown_ = false;
    bool cef_instantiated_ = false;
};

#endif // MAINWINDOW_H

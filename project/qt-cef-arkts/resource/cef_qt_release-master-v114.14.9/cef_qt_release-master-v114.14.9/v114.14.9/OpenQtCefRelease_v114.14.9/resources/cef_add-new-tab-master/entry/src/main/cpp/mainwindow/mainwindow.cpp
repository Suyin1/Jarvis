/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#include "mainwindow.h"

#include <QDebug>
#include <QGestureEvent>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPinchGesture>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QUrl>

#include "ohos/adapter_c/adapter_c.h"
#include <thread>

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "testcef/TCSimpleApp.h"
#include "testcef/TCSimpleRender.h"

using namespace EMBEDDED_WINDOW_ADAPTER;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) 
{
    handler_ = new TCSimpleHandler(false);
    this->resize(QSize(1500, 1200));
    chromiumMainthread();
    InitControls();
}

MainWindow::~MainWindow() {}

void MainWindow::chromiumMainthread() 
{
    const std::vector<std::string> args = {
        "cef",
        "--use-gl=egl",
        "--enable-features=UseOzonePlatform",
        "--ozone-platform=ohos",
        "--enable-logging",
        "--v=0",
        "--ozone-dump-file=/data/storage/el2/base/cache/",
        "--log-file=/data/storage/el2/base/cache/test.log",
        "--in-process-gpu",
        "--no-zygote",
        "--log-net-log",
        "--bundle-installation-dir=/data/storage/el1/bundle/entry/resources/resfile/",
        "--content-shell-host-window-size=1139x654",
        "--user-data-dir=/data/storage/el2/base/files/",
        "--enable-media-stream=true",
        "--enable-speech-input=true",
        "--enable-print-preview",
		"--force-renderer-accessibility=basic",
        "--lang=zh-CN",
        "--remote-debugging-port=9222",
        "--remote-allow-origins=http://localhost:9222",
    };
    std::thread chromiumMainthread([args, this]() {
        std::vector<char *> argv_cstr;
        for (const auto &arg : args) {
            argv_cstr.push_back(const_cast<char *>(arg.c_str()));
        }
        MainWindow::InitCef(argv_cstr.size(), argv_cstr.data());
    });
    chromiumMainthread.detach();
}

void MainWindow::InitControls() 
{
    QWidget* toolbar = new QWidget(this);
    toolbar->setFixedHeight(100);
    toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setSpacing(10);
    toolbarLayout->setContentsMargins(50, 10, 50, 10);

    QLineEdit *addressBar = new QLineEdit(this);
    addressBar->setStyleSheet("QLineEdit {"
                              "   background-color: #f0f0f0;"
                              "   border: 2px solid #ccc;"
                              "   border-radius: 5px;"
                              "   padding: 5px;"
                              "}"
                              "QLineEdit:focus {"
                              "   border: 2px solid #0078d7;"
                              "}");
    addressBar->setPlaceholderText("Please input url here...");
    QPushButton *go = new QPushButton("go", this);
    QPushButton *show = new QPushButton("show", this);
    QPushButton *hide = new QPushButton("hide", this);
    QPushButton *back = new QPushButton("back", this);
    QPushButton *forward = new QPushButton("forward", this);
    QPushButton *newWnd = new QPushButton("newWnd", this);

    QWidget* buttonContainer = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    for (auto* btn : {go, show, hide, back, forward, newWnd}) {
        btn->setFixedSize(150, 50);
        buttonLayout->addWidget(btn);
    }
    toolbarLayout->addWidget(addressBar, 6);
    toolbarLayout->addWidget(buttonContainer, 4);
    setCentralWidget(toolbar);

    connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::OnJump);
    connect(addressBar, &QLineEdit::editingFinished, this,
            [this, addressBar]() { url_ = addressBar->text().trimmed(); });
    connect(go, &QPushButton::clicked, this, &MainWindow::OnJump);
    connect(show, &QPushButton::clicked, this, [this]() {
        if (browserCreated_ && handler_) {
            EWAdapterC *adapter_c = EWAdapterC::getInstance();
            if (adapter_c) {
                adapter_c->showNode((Node *)GetBrowserWindowHandle());
            }
        }
    });
    connect(hide, &QPushButton::clicked, this, [this]() {
        if (browserCreated_ && handler_) {
            EWAdapterC *adapter_c = EWAdapterC::getInstance();
            if (adapter_c) {
                adapter_c->hideNode((Node *)GetBrowserWindowHandle());
            }
        }
    });
    connect(back, &QPushButton::clicked, this, [this]() {
        if (browserCreated_ && handler_) {
            handler_->GoBack(GetBrowserId());
        }
    });
    connect(forward, &QPushButton::clicked, this, [this]() {
        if (browserCreated_ && handler_) {
            handler_->GoForward(GetBrowserId());
        }
    });
    connect(newWnd, &QPushButton::clicked, this, [this]() {
        if (!cefInstantiated_ || !handler_) return; 
        if (newWindow_ == nullptr) { 
            newWindow_ = new NewWindow(handler_, this);
            connect(newWindow_, &NewWindow::windowClosed, this, &MainWindow::OnChildWindowClosed);
            newWindow_->show();
        } else {
            newWindow_->addNewTab();
            newWindow_->raise(); // 窗口置顶（体验优化）
        }
    });
}

void MainWindow::OnChildWindowClosed(NewWindow* window) 
{
    if (window == newWindow_) {
        newWindow_ = nullptr;
    }
}

int MainWindow::InitCef(int argc, char *argv[]) 
{
    CefMainArgs main_args(argc, argv);
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);
    CefRefPtr<CefApp> app;
    TCSimpleCefApp::ProcessType process_type = TCSimpleCefApp::GetProcessType(command_line);
    LOG(INFO) << "call API: GetProcessType end: " << __LINE__ << "  process_type: " << process_type;
    if (process_type == TCSimpleCefApp::RendererProcess || process_type == TCSimpleCefApp::ZygoteProcess) {
        app = new TCSimpleRender();
    } else {
        app = new TCSimpleApp();
    }
    int exitCode = CefExecuteProcess(main_args, app, nullptr);
    LOG(INFO) << "call API: CefExecuteProcess end: " << __LINE__ << "  return: " << exitCode;
    if (exitCode >= 0) {
        return exitCode;
    }
    CefSettings settings;
    TCSimpleApp::PopulateSettings(command_line, settings);
    if (command_line->HasSwitch("enable-chrome-runtime")) {
        settings.chrome_runtime = true;
    }
#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif
    bool bret = CefInitialize(main_args, settings, app.get(), nullptr);
    LOG(INFO) << "call API: CefInitialize end: " << __LINE__ << "  return: " << bret;
    cefInstantiated_ = bret;
    if (bret) LoadURL(default_url_);
    CefRunMessageLoop();
    LOG(INFO) << "call API: CefRunMessageLoop end: " << __LINE__;
    CefShutdown();
    LOG(INFO) << "call API: CefShutdown end: " << __LINE__;
    shutdown_ = true;
    return 0;
}

void MainWindow::OnJump() 
{
    if (!cefInstantiated_) {
        return;
    }
    QString url = url_;
    if (url.isEmpty()) {
        url = default_url_;
    } else {
        if (!url.startsWith("http://") && !url.startsWith("https://")) {
            url = "http://" + url;
        }
    }
    LoadURL(url);
}

bool MainWindow::CreateBrowser(const QString &url) 
{
    if (!handler_) {
        return false;
    }
    return handler_->CreateBrowserForWindow(
        GetWinId(), CefRect(10, 100, width() - 20, height() - 110),
        url.toStdString(),
        [this](int64_t identifier) { this->onBrowserCreated(identifier); });
}

void MainWindow::LoadURL(const QString &url) 
{
    if (browserCreated_ && handler_) {
        handler_->LoadURL(GetBrowserId(), url.toStdString());
    } else {
        browserCreated_ = CreateBrowser(url);
    }
    setFocus(Qt::ActiveWindowFocusReason);
}

long MainWindow::GetWinId() 
{
    return (CefWindowHandle)this->winId();
}

int64_t MainWindow::GetBrowserId() 
{
    return identifier_;
}

long MainWindow::GetBrowserWindowHandle() 
{
    return handler_ ? handler_->getBrowserWindowHandle(GetBrowserId()) : 0;
}

void MainWindow::onBrowserCreated(int64_t identifier) 
{
    if (identifier_ == 0) {
        identifier_ = identifier;
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) 
{
    QMainWindow::resizeEvent(event);
    if (browserCreated_ && handler_) {
        EWAdapterC *adapter_c = EWAdapterC::getInstance();
        if (adapter_c) {
            QSize newSize = event->size();
            adapter_c->moveNode((Node *)GetBrowserWindowHandle(), 10, 100);
            adapter_c->resizeNode((Node *)GetBrowserWindowHandle(), newSize.width() - 20,
                                  newSize.height() - 110);
        }
    }
}

void MainWindow::closeEvent(QCloseEvent *event) 
{
    if (cefInstantiated_) {
        if (browserCreated_ && handler_) {
            // 关闭子窗口（如果存在）
            if (newWindow_) {
                newWindow_->close();
            }
            if (!shutdown_) {
                closeAllBrowsers(event);
                event->ignore();
            }
            EWAdapterC *adapter_c = EWAdapterC::getInstance();
            if (adapter_c) {
                adapter_c->removeNode((Node *)GetBrowserWindowHandle());
            }
        } else {
            if (!shutdown_) {
                CefQuitMessageLoop();
            }
        }
    }
}

void MainWindow::closeAllBrowsers(QCloseEvent *event) 
{
    QTimer::singleShot(100, this, [this, event]() { checkCefShutdown(event); });
}

void MainWindow::checkCefShutdown(QCloseEvent *event) 
{
    if (shutdown_) {
        event->accept();
        QMainWindow::close();
    } else {
        closeAllBrowsers(event);
    }
}

void MainWindow::focusInEvent(QFocusEvent *event) 
{
    QMainWindow::focusInEvent(event);
    qDebug() << "lxh---focusInEvent-----type:" << event->type();
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (adapter_c) {
        adapter_c->requestFocus((Node *)GetBrowserWindowHandle());
    }
}

void MainWindow::focusOutEvent(QFocusEvent *event) 
{
    QMainWindow::focusOutEvent(event);
    qDebug() << "lxh---focusOutEvent-----type:" << event->type();
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (adapter_c) {
        adapter_c->loseFocus((Node *)GetBrowserWindowHandle());
    }
}
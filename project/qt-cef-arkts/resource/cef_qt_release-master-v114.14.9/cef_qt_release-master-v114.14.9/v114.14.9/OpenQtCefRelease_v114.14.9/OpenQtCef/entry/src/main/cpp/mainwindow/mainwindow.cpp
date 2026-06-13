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
#include <QPointer>

#include "ohos/adapter_c/adapter_c.h"
#include <thread>

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "testcef/TCSimpleApp.h"
#include "testcef/TCSimpleRender.h"

using namespace EMBEDDED_WINDOW_ADAPTER;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    handler_ = new TCSimpleHandler(false);
    this->resize(QSize(1500, 1200));

    chromiumMainthread();
    initControls();
}

MainWindow::~MainWindow() {}

void MainWindow::chromiumMainthread() {
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
        "--remote-debugging-port=9222",                 // for remote dubug and autotest
        "--remote-allow-origins=http://localhost:9222", // for remote dubug and autotest
    };
    std::thread chromiumMainthread([args, this]() {
        std::vector<char *> argv_cstr;
        for (const auto &arg : args) {
            argv_cstr.push_back(const_cast<char *>(arg.c_str()));
        }
        MainWindow::initCef(argv_cstr.size(), argv_cstr.data());
    });
    chromiumMainthread.detach();
}

void MainWindow::initControls() {
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
    QPushButton *newWnd = new QPushButton("独立窗口", this);
    QPushButton *newWnd2 = new QPushButton("子窗口", this);
    QPushButton *dlg = new QPushButton("独立对话框", this);
    QPushButton *dlg2 = new QPushButton("子对话框", this);

    QWidget* buttonContainer = new QWidget(this);
    QHBoxLayout* buttonLayout = new QHBoxLayout(buttonContainer);
    buttonLayout->setSpacing(10);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    for (auto* btn : {go, show, hide, back, forward, newWnd, newWnd2, dlg, dlg2}) {
        btn->setFixedSize(150, 50);
        buttonLayout->addWidget(btn);
    }
    toolbarLayout->addWidget(addressBar, 6);
    toolbarLayout->addWidget(buttonContainer, 4);
    setCentralWidget(toolbar);

    connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::onJump);
    connect(addressBar, &QLineEdit::editingFinished, this,
            [this, addressBar]() { url_ = addressBar->text().trimmed(); });
    connect(go, &QPushButton::clicked, this, &MainWindow::onJump);
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
    connect(newWnd, &QPushButton::clicked, this, [=]() {
        if (handler_) {
            createWindowsSequentially(false);
        }
    });
    connect(newWnd2, &QPushButton::clicked, this, [=]() {
        if (handler_) {
            createWindowsSequentially(true);
        }
    });
    connect(dlg, &QPushButton::clicked, this, [=]() {
        if (testDialog_ == nullptr) {
            testDialog_ = new TestDialog(handler_, nullptr);
            connect(testDialog_, &TestDialog::windowClosed, this, [=]() {
                testDialog_ = nullptr;
            });
        }
        if (testDialog_) {
            testDialog_->activeChildWindow();
            testDialog_->show();
            testDialog_->activateWindow();
        }
    });
    connect(dlg2, &QPushButton::clicked, this, [=]() {
        if (testDialog2_ == nullptr) {
            testDialog2_ = new TestDialog(handler_, this);
            connect(testDialog2_, &TestDialog::windowClosed, this, [=]() {
                testDialog2_ = nullptr;
            });
        }
        if (testDialog2_) {
            testDialog2_->activeChildWindow();
            testDialog2_->show();
            testDialog2_->activateWindow();
        }
    });
}

void MainWindow::createWindowsSequentially(bool is_child) {
    currentCreatingIndex_ = 0;
    createNextWindow(is_child);
}

void MainWindow::createNextWindow(bool is_child) {
    // Only for test.
    if (currentCreatingIndex_ >= 1) {
        return;
    }
    QPoint pt = pos();
    pt.setX(pt.x() + 40 * (currentCreatingIndex_ + 1));
    pt.setY(pt.y() + 40 * (currentCreatingIndex_ + 1));
    NewWindow* window = new NewWindow(handler_, currentCreatingIndex_, pt, size(), is_child ? this : nullptr);
    if (window) {
        connect(window, &NewWindow::windowClosed, this, &MainWindow::onChildWindowClosed);
        connect(window, &NewWindow::browserReady, this, &MainWindow::onChildBrowserReady);
        connect(window, &NewWindow::browserReady, this, [=]() {
            newWindows_.push_back(window);
            currentCreatingIndex_++;
            createNextWindow(is_child);
        });
        window->LoadURL("https://cn.bing.com/");
//        window->LoadURL("https://ams.haitaibrowser.com:8000/ohcef/newdemo/input-text.html");
    }
}

void MainWindow::onChildWindowClosed(NewWindow* window) {
    auto it = std::find(newWindows_.begin(), newWindows_.end(), window);
    if (it != newWindows_.end()) {
        newWindows_.erase(it);
    }
}

void MainWindow::onChildBrowserReady(NewWindow* window, qint64 identifier) {
    if (window) {
        window->setWindowTitle("New Window " + QString::number(window->GetWinId()) + " -- " + QString::number(identifier));
        window->show();
    }
}

int MainWindow::initCef(int argc, char *argv[]) {
    // Provide CEF with command-line arguments.
    CefMainArgs main_args(argc, argv);

    // Parse command-line arguments for use in this method.
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    // SimpleApp implements application-level callbacks for the browser process.
    // It will create the first browser instance in OnContextInitialized() after
    // CEF has initialized.
    // CefRefPtr<SimpleApp> app(new SimpleApp);
    CefRefPtr<CefApp> app;
    TCSimpleCefApp::ProcessType process_type = TCSimpleCefApp::GetProcessType(command_line);
    LOG(INFO) << "call API: GetProcessType end: " << __LINE__ << "  process_type: " << process_type;
    if (process_type == TCSimpleCefApp::RendererProcess || process_type == TCSimpleCefApp::ZygoteProcess) {
        app = new TCSimpleRender();
    } else {
        app = new TCSimpleApp();
    }

    // CEF applications have multiple sub-processes (render, GPU, etc) that share
    // the same executable. This function checks the command-line and, if this is
    // a sub-process, executes the appropriate logic.
    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    LOG(INFO) << "call API: CefExecuteProcess end: " << __LINE__ << "  return: " << exit_code;
    if (exit_code >= 0) {
        // The sub-process has completed so return here.
        return exit_code;
    }

    // Specify CEF global settings here.
    CefSettings settings;
    TCSimpleApp::PopulateSettings(command_line, settings);

    // turn on below for no-effect in web_loader.cpp
    // settings.multi_threaded_message_loop=false;
    // settings.external_message_pump=true;

    if (command_line->HasSwitch("enable-chrome-runtime")) {
        // Enable experimental Chrome runtime. See issue #2969 for details.
        settings.chrome_runtime = true;
    }

// When generating projects with CMake the CEF_USE_SANDBOX value will be defined
// automatically. Pass -DUSE_SANDBOX=OFF to the CMake command-line to disable
// use of the sandbox.
#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif
    // enable osr
    // settings.windowless_rendering_enabled = true;
//    CefString(&settings.browser_subprocess_path) = "/data/storage/el1/bundle/libs/arm64/libsubprocess.so";

    // Initialize CEF for the browser process.
    bool bret = CefInitialize(main_args, settings, app.get(), nullptr);
    LOG(INFO) << "call API: CefInitialize end: " << __LINE__ << "  return: " << bret;
    cef_instantiated_ = bret;
    // Load here, due to cef init ended.
    if (bret) LoadURL(default_url_);
    
    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();
    LOG(INFO) << "call API: CefRunMessageLoop end: " << __LINE__;

    // Shut down CEF. 关闭注释后应用直接退出
    CefShutdown();
    LOG(INFO) << "call API: CefShutdown end: " << __LINE__;
    shutdown_ = true;

    return 0;
}

void MainWindow::onJump() {
    if (!cef_instantiated_) {
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

bool MainWindow::CreateBrowser(const QString &url) {
    if (!handler_) {
        return false;
    }
    return handler_->CreateBrowserForWindow(
        GetWinId(), CefRect(10, 100, width() - 20, height() - 110),
        url.toStdString(),
        [this](int64_t identifier) { this->onBrowserCreated(identifier); });
}

void MainWindow::LoadURL(const QString &url) {
    if (browserCreated_ && handler_) {
        handler_->LoadURL(GetBrowserId(), url.toStdString());
    } else {
        browserCreated_ = CreateBrowser(url);
    }
    QMetaObject::invokeMethod(this,[this]() {
        this->setFocus(Qt::ActiveWindowFocusReason);
    });
}

long MainWindow::GetWinId() {
    return (CefWindowHandle)this->winId();
}

int64_t MainWindow::GetBrowserId() {
    return identifier_;
}

long MainWindow::GetBrowserWindowHandle() {
    return handler_ ? handler_->getBrowserWindowHandle(GetBrowserId()) : 0;
}

void MainWindow::onBrowserCreated(int64_t identifier) {
    if (identifier_ == 0) {
        identifier_ = identifier;
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
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

void MainWindow::closeEvent(QCloseEvent *event) {
    if (testDialog_) {
        testDialog_->close();
        testDialog_ = nullptr;
    }
    if (testDialog2_) {
        testDialog2_->close();
        testDialog2_ = nullptr;
    }
    // Fix: If CEF is initialized successful but no browser created, CefShutdown is still required
    if (cef_instantiated_) {
        if (browserCreated_ && handler_) {
            auto windowsToClose = std::move(newWindows_);
            for (auto& window : windowsToClose) {
                if (window) window->close();
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

void MainWindow::closeAllBrowsers(QCloseEvent *event) {
    QPointer<MainWindow> weakThis(this);
    QTimer::singleShot(100, [weakThis, event]() {
        if (weakThis) {
            weakThis->checkCefShutdown(event);
        }
    });
}

void MainWindow::checkCefShutdown(QCloseEvent *event) {
    if (shutdown_) {
        event->accept();
        QMainWindow::close();
    } else {
        closeAllBrowsers(event);
    }
}

void MainWindow::focusInEvent(QFocusEvent *event) {
    QMainWindow::focusInEvent(event);
    qDebug() << "lxh---focusInEvent-----type:" << event->type();
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (adapter_c) {
        adapter_c->requestFocus((Node *)GetBrowserWindowHandle());
    }
}



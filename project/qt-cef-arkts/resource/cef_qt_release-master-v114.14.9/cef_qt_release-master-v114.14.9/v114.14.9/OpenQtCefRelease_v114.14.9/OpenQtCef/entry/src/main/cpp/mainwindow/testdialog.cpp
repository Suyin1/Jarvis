//
// Created on 2025/12/24.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".
// 测试对话框 — CEF JavaScript 测试对话框的实现 (JS 调用测试)

#include "testdialog.h"

#include <QApplication>
#include <QDebug>
#include <QHBoxLayout>
#include <QPushButton>

TestDialog::TestDialog(CefRefPtr<TCSimpleHandler> handler, QWidget *parent)
    : QMainWindow(parent), parent_(parent), handler_(handler) {
    setWindowTitle(parent_ ? "子对话框" : "独立对话框");
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowFlags(windowFlags() |
                   Qt::Window |
                   Qt::WindowSystemMenuHint |
                   Qt::WindowCloseButtonHint |
                   Qt::WindowMinMaxButtonsHint);
    setFixedSize(1500, 1200);
    initControls();
}

TestDialog::~TestDialog() {}

void TestDialog::initControls() {
    if (handler_ == nullptr) return;
    QWidget* toolbar = new QWidget(this);
    toolbar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* mainLayout = new QVBoxLayout(toolbar);
    mainLayout->setSpacing(20);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    int miniWidth = 260;
    int miniHeight = 50;
    
    QWidget* firstRow = new QWidget(toolbar);
    QHBoxLayout* firstRowLayout = new QHBoxLayout(firstRow);
    firstRowLayout->setSpacing(20);
    firstRow->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QPushButton *show = new QPushButton("显示所有创建的窗口", firstRow);
    QPushButton *hide = new QPushButton("隐藏所有创建的窗口", firstRow);
    show->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    hide->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    show->setMinimumSize(miniWidth, miniHeight);
    hide->setMinimumSize(miniWidth, miniHeight);
    firstRowLayout->addWidget(show);
    firstRowLayout->addWidget(hide);
    
    QWidget* buttonGrid = new QWidget(toolbar);
    QGridLayout* gridLayout = new QGridLayout(buttonGrid);
    gridLayout->setSpacing(15);
    buttonGrid->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    
    QPushButton *dlg = new QPushButton(parent_ ? "创建子对话框" : "创建独立对话框", buttonGrid);
    QPushButton *newWnd1 = new QPushButton(parent_ ? "创建子窗口" : "创建独立窗口", buttonGrid);
    QPushButton *newWnd2 = new QPushButton(parent_ ? "创建隐藏子窗口" : "创建隐藏独立窗口", buttonGrid);
    // add more...
    QList<QPushButton*> buttons = {dlg, newWnd1, newWnd2};
    for (QPushButton* button : buttons) {
        button->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        button->setMinimumSize(miniWidth, miniHeight);
    }
    int maxColumns = 3;
    for (int i = 0; i < buttons.size(); ++i) {
        int row = i / maxColumns;
        int column = i % maxColumns;
        gridLayout->addWidget(buttons[i], row, column);
    }
    for (int col = 0; col < maxColumns; ++col) {
        gridLayout->setColumnStretch(col, 1);
    }
    
    mainLayout->addWidget(firstRow);
    mainLayout->addWidget(buttonGrid);
    mainLayout->setStretch(0, 1);
    mainLayout->setStretch(1, 3);
    setCentralWidget(toolbar);

    connect(dlg, &QPushButton::clicked, this, [=]() {
        TestDialog* testDialog = new TestDialog(nullptr, parent_ ? this : nullptr);
        if (testDialog) {
            testDialog->show();
            testDialog->activateWindow();
        }
    });
    connect(newWnd1, &QPushButton::clicked, this, [=]() {
        if (handler_) {
            NewWindow* window = new NewWindow(handler_, 0, pos(), size(), parent_ ? this : nullptr);
            if (window) {
                window->LoadURL("https://www.baidu.com");
                // 测试创建独立、子窗口，验证关闭cef窗口，导致创建他的上级窗口(主窗口)会被销毁
                connect(window, &NewWindow::windowClosed, this, &TestDialog::onChildWindowClosed);
                connect(window, &NewWindow::browserReady, this, [=]() {
                    testWindows_.push_back(window);
                    window->setWindowTitle(newWnd1->text() + " " + QString::number(window->GetWinId()));
                    window->show();
                    window->activateWindow();
                });
            }
        }
    });
    connect(newWnd2, &QPushButton::clicked, this, [=]() {
        if (handler_) {
            NewWindow* window = new NewWindow(handler_, 0, pos(), size(), parent_ ? this : nullptr);
            if (window) {
                // 测试创建隐藏独立、子窗口，再显示，验证白屏现象
                window->LoadURL("https://www.baidu.com");
                window->hide();
                connect(window, &NewWindow::windowClosed, this, &TestDialog::onChildWindowClosed);
                connect(window, &NewWindow::browserReady, this, [=]() {
                    testWindows_.push_back(window);
                    window->setWindowTitle(newWnd2->text() + " " + QString::number(window->GetWinId()));
                });
            }
        }
    });
    connect(show, &QPushButton::clicked, this, [=]() {
        for (const auto& window : testWindows_) {
            if (window) {
                window->show();
            }
        }
    });
    connect(hide, &QPushButton::clicked, this, [=]() {
        for (const auto& window : testWindows_) {
            if (window) {
                window->hide();
            }
        }
    });
}

void TestDialog::onChildWindowClosed(NewWindow* window) {
    qDebug() << "lxh-----------TestDialog::onChildWindowClosed--------" << window << " " << testWindows_.size();
    auto it = std::find(testWindows_.begin(), testWindows_.end(), window);
    if (it != testWindows_.end()) {
        testWindows_.erase(it);
    }
}

void TestDialog::closeEvent(QCloseEvent *event) {
    if (isClosing_) {
        return;
    }
    isClosing_ = true;
    qDebug() << "lxh-----------TestDialog::closeEvent--------" << testWindows_.size();
    auto windowsToClose = std::move(testWindows_);
    for (auto& window : windowsToClose) {
        if (window) window->close();
    }
    emit windowClosed(this);
    isClosing_ = false;
}

void TestDialog::showEvent(QShowEvent *event) {
    QMainWindow::showEvent(event);
}

void TestDialog::activeChildWindow() {
    for (const auto& window : testWindows_) {
        if (window) {
            bool isMinimized = window->windowState() & Qt::WindowMinimized;
            if (!isMinimized) {
                window->show();
                window->activateWindow();
            }
        }
    }
}

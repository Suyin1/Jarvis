/*
 * Copyright (c) Huawei Technologies Co., Ltd. 2026-2026. All rights reserved.
 */
#include "newwindow.h"

#include <QDebug>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "ohos/adapter_c/adapter_c.h"

using namespace EMBEDDED_WINDOW_ADAPTER;

NewWindow::NewWindow(CefRefPtr<TCSimpleHandler> handler, QWidget *parent) : QMainWindow(parent) 
{
    setWindowTitle("Tab Browser Window"); 
    setMinimumSize(1000, 800); 
    handler_ = handler;
    
    tabWidget_ = new QTabWidget(this);
    tabWidget_->setTabsClosable(true); 
    tabWidget_->setMovable(true);     
    setCentralWidget(tabWidget_);     
    
    connect(tabWidget_, &QTabWidget::tabCloseRequested, this, &NewWindow::onTabCloseRequested);
    
    addNewTab(defaultTabUrl_);
    
    if (parent) {
        resize(parent->size());
        move(parent->pos());
    }
}

NewWindow::~NewWindow() {}

int NewWindow::getTabIndexByWidget(QWidget* pageWidget) 
{
    if (!pageWidget) return -1;
    for (int i = 0; i < tabInfoList_.size(); ++i) {
        if (tabInfoList_[i].pageWidget == pageWidget) {
            return i; 
        }
    }
    return -1;
}

void NewWindow::addNewTab(const QString& url) 
{
    if (!handler_) return;
    QWidget* pageWidget = new QWidget(tabWidget_);
    pageWidget->setLayout(new QVBoxLayout(pageWidget));
    pageWidget->layout()->setContentsMargins(0, 0, 0, 0);
    
    int newTabIndex = tabWidget_->addTab(pageWidget, QString("Tab %1").arg(tabInfoList_.size() + 1));
    tabWidget_->setCurrentIndex(newTabIndex); 
    
    TabInfo tabInfo;
    tabInfo.pageWidget = pageWidget;
    tabInfoList_.append(tabInfo);
    
    QTimer::singleShot(100, this, [this, newTabIndex, url]() {
        createTabBrowser(newTabIndex, url);
    });
}

bool NewWindow::createTabBrowser(int tabIndex, const QString &url) 
{
    // 先通过视觉索引拿到pageWidget，再确认真实索引（加固）
    QWidget* page = tabWidget_->widget(tabIndex);
    int realIndex = getTabIndexByWidget(page);
    if (realIndex < 0 || realIndex >= tabInfoList_.size() || !handler_) {
        return false;
    }
    TabInfo& tabInfo = tabInfoList_[realIndex];
    if (tabInfo.browserCreated) return true;

    return handler_->CreateBrowserForWindow(
        GetTabWinId(realIndex),
        CefRect(0, 0, tabInfo.pageWidget->width(), tabInfo.pageWidget->height()),
        url.toStdString(),
        [this, realIndex](int64_t identifier) {
            this->onBrowserCreated(realIndex, identifier);
        }
    );
}

void NewWindow::onBrowserCreated(int tabIndex, int64_t identifier) 
{
    if (tabIndex < 0 || tabIndex >= tabInfoList_.size()) return;
    TabInfo& tabInfo = tabInfoList_[tabIndex];
    tabInfo.browserId = identifier;
    tabInfo.browserCreated = true;
    tabInfo.windowHandle = handler_->getBrowserWindowHandle(identifier);
}

// 给指定Tab加载URL
void NewWindow::LoadURL(int tabIndex, const QString &url) 
{
    if (tabIndex < 0 || tabIndex >= tabInfoList_.size() || !handler_) return;
    TabInfo& tabInfo = tabInfoList_[tabIndex];
    if (tabInfo.browserCreated) {
        handler_->LoadURL(tabInfo.browserId, url.toStdString());
    } else {
        createTabBrowser(tabIndex, url);
    }
}

void NewWindow::onTabCloseRequested(int tabVisualIndex) 
{
    QWidget* pageWidget = tabWidget_->widget(tabVisualIndex);
    int realIndex = getTabIndexByWidget(pageWidget);
    if (realIndex < 0 || realIndex >= tabInfoList_.size()) return;

    TabInfo& tabInfo = tabInfoList_[realIndex];
    if (tabInfo.browserCreated && tabInfo.windowHandle) {
        EWAdapterC *adapter_c = EWAdapterC::getInstance();
        if (adapter_c) {
            adapter_c->removeNode((Node *)tabInfo.windowHandle);
        }
    }
    
    tabWidget_->removeTab(tabVisualIndex);
    delete tabInfo.pageWidget;
    tabInfo.pageWidget = nullptr;
    tabInfoList_.removeAt(realIndex);
    
    if (tabInfoList_.isEmpty()) {
        close();
    }
}

long NewWindow::GetTabWinId(int tabIndex) 
{
    if (tabIndex < 0 || tabIndex >= tabInfoList_.size()) return 0;
    return (CefWindowHandle)tabInfoList_[tabIndex].pageWidget->winId();
}

void NewWindow::resizeEvent(QResizeEvent *event) 
{
    QMainWindow::resizeEvent(event);
    if (!tabWidget_ || tabInfoList_.isEmpty() || !handler_) return;

    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (!adapter_c) return;
    for (int i = 0; i < tabInfoList_.size(); ++i) {
        TabInfo& tabInfo = tabInfoList_[i];
        if (tabInfo.browserCreated && tabInfo.windowHandle) {
            QWidget* page = tabInfo.pageWidget;
            adapter_c->resizeNode((Node *)tabInfo.windowHandle, page->width(), page->height());
        }
    }
}

void NewWindow::closeEvent(QCloseEvent *event) 
{
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    for (TabInfo& tabInfo : tabInfoList_) {
        if (tabInfo.browserCreated && tabInfo.windowHandle && adapter_c) {
            adapter_c->removeNode((Node *)tabInfo.windowHandle);
        }
        delete tabInfo.pageWidget;
        tabInfo.pageWidget = nullptr;
    }
    tabInfoList_.clear();
    QMainWindow::closeEvent(event);
    emit windowClosed(this);
}
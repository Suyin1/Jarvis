//
// Created on 2025/7/16.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "newwindow.h"

#include <QDebug>
#include <QResizeEvent>

#include "ohos/adapter_c/adapter_c.h"
#include <QTimer>
#include <QThread>
#include <QPointer>
using namespace EMBEDDED_WINDOW_ADAPTER;

NewWindow::NewWindow(CefRefPtr<TCSimpleHandler> handler, int index, QPoint pt, QSize sz, QWidget *parent) : QMainWindow(parent) {
    qWarning() << "lxh----NewWindow::NewWindow GetWinId: " << GetWinId() << "  index: " << index << "  " << this;
    setWindowTitle(parent ? "子窗口" : "独立窗口");
    setAttribute(Qt::WA_DeleteOnClose, true);
    handler_ = handler;
    this->resize(sz);
    this->move(pt);
    QTimer::singleShot(300, this, [this]() {
        QPointer<NewWindow> self(this);
        if (self) {
            qDebug() << "NewWindow: 发射 browserReady 信号";
            emit browserReady(self, static_cast<qint64>(GetBrowserId()));
        }
    });
//    LoadURL(url_);
}

NewWindow::~NewWindow() {}


bool NewWindow::CreateBrowser(const QString& url) {
    if (!handler_) {
        return false;
    }

    return handler_->CreateBrowserForWindow(
        GetWinId(), CefRect(0, 0, width(), height()),
        url.toStdString(),
        [this](int64_t identifier) { this->onBrowserCreated(identifier); });
}

void NewWindow::LoadURL(const QString &url) {
    if (browserCreated_ && handler_) {
        handler_->LoadURL(GetBrowserId(), url.toStdString());
    } else {
        browserCreated_ = CreateBrowser(url);
    }
    setFocus(Qt::ActiveWindowFocusReason);
}

long NewWindow::GetWinId() {
    return (CefWindowHandle)this->winId();
}

int64_t NewWindow::GetBrowserId() {
    return identifier_;
}

long NewWindow::GetBrowserWindowHandle() {
    return handler_ ? handler_->getBrowserWindowHandle(GetBrowserId()) : 0;
}

void NewWindow::onBrowserCreated(int64_t identifier) {
    if (identifier_ == 0) {
        identifier_ = identifier;
        /*if (!this) return;
        QMetaObject::invokeMethod(this, [this, identifier]() {
            if (this) {
                emit browserReady(this, static_cast<qint64>(identifier));
            }
        }, Qt::QueuedConnection);*/
    }
}

void NewWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    if (browserCreated_ && handler_) {
        EWAdapterC *adapter_c = EWAdapterC::getInstance();
        if (adapter_c) {
            QSize newSize = event->size();
            adapter_c->moveNode((Node *)GetBrowserWindowHandle(), 0, 0);
            adapter_c->resizeNode((Node *)GetBrowserWindowHandle(), newSize.width(),
                                  newSize.height());
        }
    }
}

void NewWindow::closeEvent(QCloseEvent *event) {
    QThread::msleep(300);
    QMainWindow::closeEvent(event);
    if (browserCreated_ && handler_) {
        EWAdapterC *adapter_c = EWAdapterC::getInstance();
        if (adapter_c) {
            adapter_c->removeNode((Node *)GetBrowserWindowHandle());
        }
    }
    emit windowClosed(this);
}

void NewWindow::focusInEvent(QFocusEvent *event) {
    QMainWindow::focusInEvent(event);
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (adapter_c) {
        adapter_c->requestFocus((Node *)GetBrowserWindowHandle());
    }
}

void NewWindow::focusOutEvent(QFocusEvent *event) {
    QMainWindow::focusOutEvent(event);
    EWAdapterC *adapter_c = EWAdapterC::getInstance();
    if (adapter_c) {
        adapter_c->loseFocus((Node *)GetBrowserWindowHandle());
    }
}
// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// CEF 渲染进程入口 — 处理页面渲染和 V8 JavaScript 绑定

#include "TCSimpleRender.h"

#include "include/base/cef_logging.h"
#include "include/cef_dom.h"
#include "include/cef_shared_memory_region.h"
#include "include/cef_shared_process_message_builder.h"
#include <sstream>
#include <string>

#include "TCSimpleRenderDelegate.h"

// static
void TCSimpleRender::CreateDelegates(DelegateSet &delegates) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    delegates.insert(new TCSimpleRenderDelegate);
}

TCSimpleRender::TCSimpleRender() {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    CreateDelegates(delegates_);
}

void TCSimpleRender::OnWebKitInitialized() {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnWebKitInitialized";
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnWebKitInitialized(this);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnWebKitInitialized";
}

void TCSimpleRender::OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnBrowserCreated";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnBrowserCreated(this, browser, extra_info);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnBrowserCreated";
}

void TCSimpleRender::OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnBrowserDestroyed";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnBrowserDestroyed(this, browser);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnBrowserDestroyed";
}

CefRefPtr<CefLoadHandler> TCSimpleRender::GetLoadHandler() {
    LOG(INFO) << "Enter CefRenderProcessHandler call API GetLoadHandler";
    CefRefPtr<CefLoadHandler> load_handler;
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end() && !load_handler.get(); ++it) {
        load_handler = (*it)->GetLoadHandler(this);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API GetLoadHandler return value:" << &load_handler;
    return load_handler;
}

void TCSimpleRender::OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                      CefRefPtr<CefV8Context> context) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnContextCreated";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnContextCreated(this, browser, frame, context);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnContextCreated";
}

void TCSimpleRender::OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                       CefRefPtr<CefV8Context> context) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnContextReleased";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnContextReleased(this, browser, frame, context);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnContextReleased";
}

void TCSimpleRender::OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                         CefRefPtr<CefV8Context> context, CefRefPtr<CefV8Exception> exception,
                                         CefRefPtr<CefV8StackTrace> stackTrace) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnUncaughtException";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnUncaughtException(this, browser, frame, context, exception, stackTrace);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnUncaughtException";
}

void TCSimpleRender::OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefDOMNode> node) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnFocusedNodeChanged";
    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end(); ++it) {
        (*it)->OnFocusedNodeChanged(this, browser, frame, node);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API OnFocusedNodeChanged";
}

bool TCSimpleRender::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                              CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    LOG(INFO) << "Enter CefRenderProcessHandler call API OnProcessMessageReceived";
    DCHECK_EQ(source_process, PID_BROWSER);

    const char *kDomTestMessage = "DOMTest.Test";
    if (message->GetName() == kDomTestMessage) {
        int test_type = message->GetArgumentList()->GetInt(0);
        LOG(INFO) << "dom call API: " << __FUNCTION__ << ": " << __LINE__ << " test_type: " << test_type;
        return true;
    }

    bool handled = false;

    DelegateSet::iterator it = delegates_.begin();
    for (; it != delegates_.end() && !handled; ++it) {
        handled = (*it)->OnProcessMessageReceived(this, browser, frame, source_process, message);
    }
    LOG(INFO) << "Leave CefRenderProcessHandler call API "
                 "OnProcessMessageReceived return value:"
              << handled;
    return handled;
}

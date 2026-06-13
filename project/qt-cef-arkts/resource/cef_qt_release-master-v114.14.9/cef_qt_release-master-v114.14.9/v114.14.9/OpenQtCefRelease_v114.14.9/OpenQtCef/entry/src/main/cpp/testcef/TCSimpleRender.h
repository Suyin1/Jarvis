// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_RENDERER_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_RENDERER_H_
#pragma once

#include <set>

#include "include/cef_app.h"
#include "TCSimpleCefApp.h"

// Client app implementation for the renderer process.
class TCSimpleRender : public TCSimpleCefApp, public CefRenderProcessHandler {
public:
    // Interface for renderer delegates. All Delegates must be returned via
    // CreateDelegates. Do not perform work in the Delegate
    // constructor. See CefRenderProcessHandler for documentation.
    class Delegate : public virtual CefBaseRefCounted {
    public:
        virtual void OnWebKitInitialized(CefRefPtr<TCSimpleRender> app) {}

        virtual void OnBrowserCreated(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefDictionaryValue> extra_info) {}

        virtual void OnBrowserDestroyed(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser) {}

        virtual CefRefPtr<CefLoadHandler> GetLoadHandler(CefRefPtr<TCSimpleRender> app) { return nullptr; }

        virtual void OnContextCreated(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {}

        virtual void OnContextReleased(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                       CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) {}

        virtual void OnUncaughtException(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                         CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context,
                                         CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace) {}

        virtual void OnFocusedNodeChanged(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) {}

        // Called when a process message is received. Return true if the message was
        // handled and should not be passed on to other handlers. Delegates
        // should check for unique message names to avoid interfering with each
        // other.
        virtual bool OnProcessMessageReceived(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                              CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                              CefRefPtr<CefProcessMessage> message) {
            return false;
        }
    };

    typedef std::set<CefRefPtr<Delegate>> DelegateSet;

    TCSimpleRender();

private:
    // Creates all of the Delegate objects.
    static void CreateDelegates(DelegateSet &delegates);

    // CefApp methods.
    CefRefPtr<CefRenderProcessHandler> GetRenderProcessHandler() override {
        LOG(INFO) << "Enter CefApp call API GetRenderProcessHandler";
        LOG(INFO) << "Leave CefApp call API GetRenderProcessHandler return value:" << this;
        return this;
    }

    // CefRenderProcessHandler methods.
    void OnWebKitInitialized() override;
    void OnBrowserCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDictionaryValue> extra_info) override;
    void OnBrowserDestroyed(CefRefPtr<CefBrowser> browser) override;
    CefRefPtr<CefLoadHandler> GetLoadHandler() override;
    void OnContextCreated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                          CefRefPtr<CefV8Context> context) override;
    void OnContextReleased(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                           CefRefPtr<CefV8Context> context) override;
    void OnUncaughtException(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context,
                             CefRefPtr<CefV8Exception> exception, CefRefPtr<CefV8StackTrace> stackTrace) override;
    void OnFocusedNodeChanged(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                              CefRefPtr<CefDOMNode> node) override;
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                  CefRefPtr<CefProcessMessage> message) override;

private:
    // Set of supported Delegates.
    DelegateSet delegates_;

    IMPLEMENT_REFCOUNTING(TCSimpleRender);
    DISALLOW_COPY_AND_ASSIGN(TCSimpleRender);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_RENDERER_H_

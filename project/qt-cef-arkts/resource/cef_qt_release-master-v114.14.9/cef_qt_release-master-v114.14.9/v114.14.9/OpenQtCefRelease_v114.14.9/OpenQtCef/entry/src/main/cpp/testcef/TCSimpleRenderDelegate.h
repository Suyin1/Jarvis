// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_COOKIE_TEST_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_COOKIE_TEST_H_

#include "include/base/cef_callback.h"
#include "include/cef_crash_util.h"
#include "include/cef_shared_process_message_builder.h"
#include "include/cef_v8.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_message_router.h"
#include <iostream>
#include <string>

#include "TCSimpleRender.h"

// used for render process

class MyCefV8Handler : public CefV8Handler {
public:
    virtual bool Execute(const CefString &name, CefRefPtr<CefV8Value> object, const CefV8ValueList &arguments,
                         CefRefPtr<CefV8Value> &retval, CefString &exception) override {
        DCHECK(CefCurrentlyOn(TID_RENDERER));

        if (arguments.size() < 2) {
            return false;
        }

        if (arguments.size() == 2 && arguments[1]->IsFunction()) {
            CefString strParam = arguments.at(0)->GetStringValue();
            CefRefPtr<CefV8Value> pCallback = arguments.at(1);

            auto it = render_callback_.find(js_callback_id_);
            if (it == render_callback_.cend()) {
                CefRefPtr<CefV8Context> context = CefV8Context::GetCurrentContext();

                CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("CallBackFunction");
                message->GetArgumentList()->SetString(0, name);
                message->GetArgumentList()->SetString(1, strParam);
                message->GetArgumentList()->SetInt(2, js_callback_id_);
                context->GetFrame()->SendProcessMessage(PID_BROWSER,
                                                        message); // JS to Browser

                render_callback_.emplace(js_callback_id_, std::make_pair(context, pCallback));

                js_callback_id_++;
                return true;
            }
        }
        return false;
    }

    // callback to JS
    bool ExecuteJSCallbackFunc(const int &js_callback_id, const int &err_code, const CefString &json_result) {
        DCHECK(CefCurrentlyOn(TID_RENDERER));

        auto it = render_callback_.find(js_callback_id);
        if (it != render_callback_.cend()) {
            auto context = it->second.first;
            auto callback = it->second.second;
            if (context.get() && callback.get()) {
                context->Enter();
                CefV8ValueList arguments;
                arguments.push_back(CefV8Value::CreateString(std::to_wstring(err_code)));
                arguments.push_back(CefV8Value::CreateString(json_result.ToWString()));
                callback->ExecuteFunction(nullptr, arguments);
                context->Exit();
                render_callback_.erase(it);
                return true;
            }
        }
        return false;
    }

private:
    IMPLEMENT_REFCOUNTING(MyCefV8Handler);

private:
    uint32 js_callback_id_ = 0;
    std::map<int, std::pair<CefRefPtr<CefV8Context>, CefRefPtr<CefV8Value>>> render_callback_;
};

// Must match the value in simple_handler.cc.
const char kFocusedNodeChangedMessage[] = "ClientRenderer.FocusedNodeChanged";

class TCSimpleRenderDelegate : public TCSimpleRender::Delegate {
public:
    TCSimpleRenderDelegate() : last_node_is_editable_(false) {}

    virtual void OnWebKitInitialized(CefRefPtr<TCSimpleRender> app) override {
        LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
        if (CefCrashReportingEnabled()) {
            // Set some crash keys for testing purposes. Keys must be defined in the
            // "crash_reporter.cfg" file. See cef_crash_util.h for details.
        }

        // Create the renderer-side router for query handling.
        CefMessageRouterConfig config;
        message_router_ = CefMessageRouterRendererSide::Create(config);

        std::string app_code = R"(
						var ClientAPI = {};
						(() => {
							ClientAPI.showMsg = function(v1, v2) {
								native function showMsg(v1, v2);
								return showMsg(v1, v2);
							};						
						})();
					)";
        if (!m_pV8handler) {
            m_pV8handler = new (std::nothrow) MyCefV8Handler();
            CefRegisterExtension("v8/extern", app_code, m_pV8handler);
        }
    }

    virtual void OnContextCreated(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                  CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override {
        LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
        message_router_->OnContextCreated(browser, frame, context);
    }

    virtual void OnContextReleased(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame, CefRefPtr<CefV8Context> context) override {
        LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
        message_router_->OnContextReleased(browser, frame, context);
    }

    virtual void OnFocusedNodeChanged(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                      CefRefPtr<CefFrame> frame, CefRefPtr<CefDOMNode> node) override {
        LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
        bool is_editable = (node.get() && node->IsEditable());
        if (is_editable != last_node_is_editable_) {
            // Notify the browser of the change in focused element type.
            last_node_is_editable_ = is_editable;
            CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create(kFocusedNodeChangedMessage);
            message->GetArgumentList()->SetBool(0, is_editable);
            frame->SendProcessMessage(PID_BROWSER, message);
        }
    }

    virtual bool OnProcessMessageReceived(CefRefPtr<TCSimpleRender> app, CefRefPtr<CefBrowser> browser,
                                          CefRefPtr<CefFrame> frame, CefProcessId source_process,
                                          CefRefPtr<CefProcessMessage> message) override {
        const CefString &message_name = message->GetName();
        LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
        if (message_name == "ExecutedCallbackToJS") {
            int callback_id = message->GetArgumentList()->GetInt(0);
            int err_code = message->GetArgumentList()->GetInt(1);
            CefString json_string = message->GetArgumentList()->GetString(2);
            return m_pV8handler->ExecuteJSCallbackFunc(callback_id, err_code, json_string);
        }
        return message_router_->OnProcessMessageReceived(browser, frame, source_process, message);
    }

private:
    bool last_node_is_editable_;

    MyCefV8Handler *m_pV8handler = nullptr;

    // Handles the renderer side of query routing.
    CefRefPtr<CefMessageRouterRendererSide> message_router_;

    DISALLOW_COPY_AND_ASSIGN(TCSimpleRenderDelegate);
    IMPLEMENT_REFCOUNTING(TCSimpleRenderDelegate);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_DEMO_COOKIE_TEST_H_

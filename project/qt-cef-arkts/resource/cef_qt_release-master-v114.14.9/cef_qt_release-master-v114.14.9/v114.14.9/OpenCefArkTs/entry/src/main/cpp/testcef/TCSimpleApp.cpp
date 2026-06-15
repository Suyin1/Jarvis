//
// Created on 2024/2/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "TCSimpleApp.h"

#include "TCSimpleHandler.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/cef_cookie.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_helpers.h"


// static
void TCSimpleApp::PopulateSettings(CefRefPtr<CefCommandLine> command_line, CefSettings &settings) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
}

TCSimpleApp::TCSimpleApp() {}

void TCSimpleApp::OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
}

void TCSimpleApp::OnContextInitialized() {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
}

CefRefPtr<CefClient> TCSimpleApp::GetDefaultClient() {
    // Called when a new browser window is created via the Chrome runtime UI.
    return TCSimpleHandler::GetInstance();
}
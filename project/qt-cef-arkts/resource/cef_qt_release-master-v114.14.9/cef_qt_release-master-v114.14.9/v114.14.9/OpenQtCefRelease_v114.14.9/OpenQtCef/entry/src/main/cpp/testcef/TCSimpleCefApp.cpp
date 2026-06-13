// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "TCSimpleCefApp.h"

#include "include/cef_command_line.h"

namespace {
// These flags must match the Chromium values.
const char kProcessType[] = "type";
const char kRendererProcess[] = "renderer";
#if defined(OS_LINUX)
const char kZygoteProcess[] = "zygote";
#endif

} // namespace

TCSimpleCefApp::TCSimpleCefApp() {}

// static
TCSimpleCefApp::ProcessType TCSimpleCefApp::GetProcessType(CefRefPtr<CefCommandLine> command_line) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    // The command-line flag won't be specified for the browser process.
    if (!command_line->HasSwitch(kProcessType)) {
        return BrowserProcess;
    }

    const std::string &process_type = command_line->GetSwitchValue(kProcessType);
    if (process_type == kRendererProcess) {
        return RendererProcess;
    }
#if defined(OS_LINUX)
    else if (process_type == kZygoteProcess) {
        return ZygoteProcess;
    }
#endif

    return OtherProcess;
}

// static
void TCSimpleCefApp::RegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    registrar->AddCustomScheme("client", CEF_SCHEME_OPTION_STANDARD | CEF_SCHEME_OPTION_CORS_ENABLED);
}

void TCSimpleCefApp::OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    RegisterCustomSchemes(registrar);
}
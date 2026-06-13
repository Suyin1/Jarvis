// Copyright (c) 2023 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_
#pragma once

#include "include/cef_app.h"

// Base class for customizing process-type-based behavior.
class TCSimpleCefApp : public CefApp {
public:
    TCSimpleCefApp();

    enum ProcessType {
        BrowserProcess,
        RendererProcess,
        ZygoteProcess,
        OtherProcess,
    };

    // Determine the process type based on command-line arguments.
    static ProcessType GetProcessType(CefRefPtr<CefCommandLine> command_line);

private:
    // Registers custom schemes. Implemented by cefclient in
    static void RegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar);

    // SimpleCefApp methods.
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

    DISALLOW_COPY_AND_ASSIGN(TCSimpleCefApp);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_

// Copyright (c) 2013 The Chromium Embedded Framework Authors. All rights
// reserved. Use of this source code is governed by a BSD-style license that
// can be found in the LICENSE file.

#include "TCSimpleApp.h"

#include <napi/native_api.h>

#include "TCSimpleHandler.h"
#include "TCSimpleRender.h"
#include "include/base/cef_logging.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"

// Entry point function for all processes.
extern "C" {
int __attribute__((visibility("default"))) CefMain(int argc, char *argv[]) {
    // Provide CEF with command-line arguments.
    CefMainArgs main_args(argc, argv);

    // Parse command-line arguments for use in this method.
    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    // print all CommandLine for test
    std::map<CefString, CefString> map_switches;
    command_line->GetSwitches(map_switches);
    std::string switches = "";
    for (const auto &pair : map_switches) {
        switches += pair.first.ToString() + (pair.second.empty() ? "" : ("=" + pair.second.ToString()));
        switches += " ";
    }
    LOG(INFO) << "call API: CefCommandLine: " << __LINE__ << " CommandLine Switches:" << switches;

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

    // Initialize CEF for the browser process.
    bool bret = CefInitialize(main_args, settings, app.get(), nullptr);
    LOG(INFO) << "call API: CefInitialize end: " << __LINE__ << "  return: " << bret;

    // Run the CEF message loop. This will block until CefQuitMessageLoop() is
    // called.
    CefRunMessageLoop();
    LOG(INFO) << "call API: CefRunMessageLoop end: " << __LINE__;

    // Shut down CEF.
    CefShutdown();
    LOG(INFO) << "call API: CefShutdown end: " << __LINE__;

    return 0;
}
}

// ============================================================
// CefSimpleOhos.cpp —— CEF 子进程入口函数（CefMain）
// 功能：作为 CEF 子进程（渲染/GPU 等）的入口点，解析命令行
//       参数、确定进程类型、执行子进程逻辑或在浏览器进程
//       中完成 CEF 初始化和消息循环。
// ============================================================

#include "TCSimpleApp.h"

#include <napi/native_api.h>

#include "TCSimpleHandler.h"
#include "TCSimpleRender.h"
#include "include/base/cef_logging.h"
#include "include/cef_browser.h"
#include "include/cef_command_line.h"
#include "include/wrapper/cef_helpers.h"

extern "C" {
int __attribute__((visibility("default"))) CefMain(int argc, char *argv[]) {
    CefMainArgs main_args(argc, argv);

    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    // 打印全部命令行开关（调试用）
    std::map<CefString, CefString> map_switches;
    command_line->GetSwitches(map_switches);
    std::string switches = "";
    for (const auto &pair : map_switches) {
        switches += pair.first.ToString() + (pair.second.empty() ? "" : ("=" + pair.second.ToString()));
        switches += " ";
    }
    LOG(INFO) << "call API: CefCommandLine: " << __LINE__ << " CommandLine Switches:" << switches;

    // 根据进程类型创建不同的 App 实例
    CefRefPtr<CefApp> app;
    TCSimpleCefApp::ProcessType process_type = TCSimpleCefApp::GetProcessType(command_line);
    LOG(INFO) << "call API: GetProcessType end: " << __LINE__ << "  process_type: " << process_type;
    if (process_type == TCSimpleCefApp::RendererProcess || process_type == TCSimpleCefApp::ZygoteProcess) {
        app = new TCSimpleRender();
    } else {
        app = new TCSimpleApp();
    }

    // 执行子进程（渲染/GPU等），若为子进程则直接返回
    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    LOG(INFO) << "call API: CefExecuteProcess end: " << __LINE__ << "  return: " << exit_code;
    if (exit_code >= 0) {
        return exit_code;
    }

    // 浏览器进程：配置设置并初始化 CEF
    CefSettings settings;
    TCSimpleApp::PopulateSettings(command_line, settings);

    if (command_line->HasSwitch("enable-chrome-runtime")) {
        settings.chrome_runtime = true;
    }

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    bool bret = CefInitialize(main_args, settings, app.get(), nullptr);
    LOG(INFO) << "call API: CefInitialize end: " << __LINE__ << "  return: " << bret;

    // 进入 CEF 消息循环（阻塞）
    CefRunMessageLoop();
    LOG(INFO) << "call API: CefRunMessageLoop end: " << __LINE__;

    CefShutdown();
    LOG(INFO) << "call API: CefShutdown end: " << __LINE__;

    return 0;
}
}

// ============================================================
// TCSimpleCefApp —— CEF 应用基类（进程类型分发）
// 功能：进程类型识别基类，根据命令行参数判断当前进程是
//       浏览器进程、渲染进程还是孵化进程。
// ============================================================

#ifndef CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_
#define CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_
#pragma once

#include "include/cef_app.h"

class TCSimpleCefApp : public CefApp {
public:
    TCSimpleCefApp();

    enum ProcessType {
        BrowserProcess,   // 浏览器主进程
        RendererProcess,  // 渲染进程
        ZygoteProcess,    // 孵化进程（用于生成子进程）
        OtherProcess,     // 其他进程
    };

    static ProcessType GetProcessType(CefRefPtr<CefCommandLine> command_line);

private:
    static void RegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar);
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override;

    DISALLOW_COPY_AND_ASSIGN(TCSimpleCefApp);
};

#endif // CEF_TESTS_CEFSIMPLE_SIMPLE_CEF_APP_H_

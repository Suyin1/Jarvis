// ============================================================
// TCSimpleApp —— CEF 浏览器进程应用类
// 功能：实现 CefApp、CefBrowserProcessHandler、CefResourceBundleHandler
//       接口，管理浏览器进程的初始化、命令行参数处理、资源包加载。
//       OnContextInitialized() 中创建首个浏览器实例。
// ============================================================

#ifndef TESTCEFDEMO_TCSIMPLEAPP_H
#define TESTCEFDEMO_TCSIMPLEAPP_H

#include "TCSimpleCefApp.h"
#include "include/cef_app.h"

class TCSimpleApp : public TCSimpleCefApp, public CefBrowserProcessHandler, public CefResourceBundleHandler {
public:
    TCSimpleApp();

    static void PopulateSettings(CefRefPtr<CefCommandLine> command_line, CefSettings &settings);

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {}

    CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() override { return this; }
    virtual bool GetLocalizedString(int string_id, CefString &string) override { return false; }
    virtual bool GetDataResource(int resource_id, void *&data, size_t &data_size) override { return false; }
    virtual bool GetDataResourceForScale(int resource_id, ScaleFactor scale_factor, void *&data,
                                         size_t &data_size) override { return false; }

    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

private:
    IMPLEMENT_REFCOUNTING(TCSimpleApp);
};

#endif // TESTCEFDEMO_TCSIMPLEAPP_H

//
// Created on 2024/2/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef TESTCEFDEMO_TCSIMPLEAPP_H
#define TESTCEFDEMO_TCSIMPLEAPP_H

#include "TCSimpleCefApp.h"
#include "include/cef_app.h"

// Implement application-level callbacks for the browser process.
class TCSimpleApp : public TCSimpleCefApp, public CefBrowserProcessHandler, public CefResourceBundleHandler {
public:
    TCSimpleApp();

    static void PopulateSettings(CefRefPtr<CefCommandLine> command_line, CefSettings &settings);

    // CefApp methods:
    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }
    void OnBeforeCommandLineProcessing(const CefString &process_type, CefRefPtr<CefCommandLine> command_line) override;
    void OnRegisterCustomSchemes(CefRawPtr<CefSchemeRegistrar> registrar) override {}

    // CefResourceBundleHandler
    CefRefPtr<CefResourceBundleHandler> GetResourceBundleHandler() override { return this; }
    virtual bool GetLocalizedString(int string_id, CefString &string) override { return false; }

    virtual bool GetDataResource(int resource_id, void *&data, size_t &data_size) override { return false; }

    virtual bool GetDataResourceForScale(int resource_id, ScaleFactor scale_factor, void *&data,
                                         size_t &data_size) override {
        return false;
    }
    // CefBrowserProcessHandler methods:
    void OnContextInitialized() override;
    CefRefPtr<CefClient> GetDefaultClient() override;

private:
    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(TCSimpleApp);
};

#endif // TESTCEFDEMO_TCSIMPLEAPP_H

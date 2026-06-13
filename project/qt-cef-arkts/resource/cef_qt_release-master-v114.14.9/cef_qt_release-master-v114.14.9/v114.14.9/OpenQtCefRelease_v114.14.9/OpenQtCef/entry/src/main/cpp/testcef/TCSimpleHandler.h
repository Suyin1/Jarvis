//
// Created on 2024/2/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#ifndef TESTCEFDEMO_TCSIMPLEHANDLER_H
#define TESTCEFDEMO_TCSIMPLEHANDLER_H

#include "include/cef_client.h"
#include "include/cef_resource_request_handler.h"
#include "include/wrapper/cef_message_router.h"
#include "include/wrapper/cef_resource_manager.h"
#include <functional>
#include <list>
#include <set>
#include <map>
#include <string>
#include <vector>

typedef std::function<void(int err_code, const std::string &result)> ReportResultFunction;
typedef std::function<void(const std::string &params, const std::string &func_name, ReportResultFunction callback)>
    CppFunction;
typedef std::map<std::pair<CefString /*function_name*/, int /*browser_id*/>, CppFunction /*function*/>
    BrowserRegisteredFunction;

class TCSimpleHandler : public CefClient,
                        public CefDisplayHandler,
                        public CefLifeSpanHandler,
                        public CefLoadHandler,
                        // override other handler here, and impl here if you needed...
                        public CefDownloadHandler,
                        public CefKeyboardHandler,
                        public CefPermissionHandler,
                        public CefPrintHandler,
                        public CefContextMenuHandler {
public:
    explicit TCSimpleHandler(bool use_views);
    ~TCSimpleHandler();

    // Provide access to the single global instance of this object.
    static TCSimpleHandler *GetInstance();

    // CefClient methods:
    virtual CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    virtual CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    virtual CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    virtual CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    virtual CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return this; }
    virtual CefRefPtr<CefPrintHandler> GetPrintHandler() override { return this; }
    virtual CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // CefContextMenuHandler methods:
    virtual void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params,
                                     CefRefPtr<CefMenuModel> model) override;
    virtual bool RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
                                CefRefPtr<CefRunContextMenuCallback> callback) override;

    // CefDisplayHandler methods:
    virtual void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) override;
    virtual void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen) override;

    // CefLifeSpanHandler methods:
    virtual bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url,
                               const CefString &target_frame_name,
                               CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture,
                               const CefPopupFeatures &popupFeatures, CefWindowInfo &windowInfo,
                               CefRefPtr<CefClient> &client, CefBrowserSettings &settings,
                               CefRefPtr<CefDictionaryValue> &extra_info, bool *no_javascript_access) override;
    virtual void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    virtual bool DoClose(CefRefPtr<CefBrowser> browser) override;
    virtual void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // CefLoadHandler methods:
    virtual void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack,
                                      bool canGoForward) override;
    virtual void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
                             const CefString &errorText, const CefString &failedUrl) override;

    // Request that all existing browser windows close.
    void CloseAllBrowsers(bool force_close);

    bool IsClosing() const { return is_closing_; }

    // Returns true if the Chrome runtime is enabled.
    static bool IsChromeRuntimeEnabled();

    // CefDownloadHandler methods
    virtual bool CanDownload(CefRefPtr<CefBrowser> browser, const CefString &url,
                             const CefString &request_method) override;
    virtual void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                                  const CefString &suggested_name,
                                  CefRefPtr<CefBeforeDownloadCallback> callback) override;
    virtual void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                                   CefRefPtr<CefDownloadItemCallback> callback) override;

    // CefKeyboardHandler methods
    virtual bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event,
                               bool *is_keyboard_shortcut) override;
    virtual bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event) override {
        return false;
    }

    // CefPrintHandler
    virtual void OnPrintStart(CefRefPtr<CefBrowser> browser) override {}
    virtual void OnPrintSettings(CefRefPtr<CefBrowser> browser, CefRefPtr<CefPrintSettings> settings,
                                 bool get_defaults) override {}
    virtual bool OnPrintDialog(CefRefPtr<CefBrowser> browser, bool has_selection,
                               CefRefPtr<CefPrintDialogCallback> callback) override {
        return false;
    }
    virtual bool OnPrintJob(CefRefPtr<CefBrowser> browser, const CefString &document_name,
                            const CefString &pdf_file_path, CefRefPtr<CefPrintJobCallback> callback) override {
        return false;
    }

    virtual void OnPrintReset(CefRefPtr<CefBrowser> browser) override {}

    virtual CefSize GetPdfPaperSize(CefRefPtr<CefBrowser> browser, int device_units_per_inch) override {
        return CefSize();
    }

    // CefPermissionHandler methods
    virtual bool OnRequestMediaAccessPermission(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                                const CefString &requesting_origin, uint32 requested_permissions,
                                                CefRefPtr<CefMediaAccessCallback> callback) override {
        return true;
    }
    virtual bool OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser, uint64 prompt_id,
                                        const CefString &requesting_origin, uint32 requested_permissions,
                                        CefRefPtr<CefPermissionPromptCallback> callback) override {
        callback->Continue(CEF_PERMISSION_RESULT_ACCEPT);
        return true;
    }
    virtual void OnDismissPermissionPrompt(CefRefPtr<CefBrowser> browser, uint64 prompt_id,
                                           cef_permission_request_result_t result) override {}

    // js call
    virtual bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                          CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;

    bool RegisterCallBackFunc(const CefString &function_name, CppFunction function, CefRefPtr<CefBrowser> browser,
                              bool replace = false);
    bool ExecuteCallbackFunc(const CefString &function_name, const CefString &params, int js_callback_id,
                             CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
    void CallCPPFunction(const std::string &params, const std::string &func_name, ReportResultFunction callback);

    long getBrowserWindowHandle(int64_t identifier);
    CefRefPtr<CefBrowser> getBrowser(int64_t identifier);
    void LoadURL(int64_t identifier, CefString url);
    void GoBack(int64_t identifier);
    void GoForward(int64_t identifier);
    // Called from the in-page context-menu JS via cefQuery. Looks up the
    // pending CefRunContextMenuCallback by `token` and dispatches the chosen
    // command_id (or -1 for cancel) back to CEF. Public so the message-router
    // handler in the .cpp can reach it.
    void DispatchContextMenuChoice(const std::string &token, int command_id);
    int GetBrowserCount() { return browser_list_.size(); }
    using BrowserCallback = std::function<void(int64_t)>;
    bool CreateBrowserForWindow(const long& window, const CefRect& rect, const CefString& url, BrowserCallback callback);

private:
    // Platform-specific implementation.
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                             const CefString& title);

    // True if the application is using the Views framework.
    const bool use_views_;

    // List of existing browser windows. Only accessed on the CEF UI thread.
    typedef std::map<long, CefRefPtr<CefBrowser>> BrowserList;
    BrowserList browser_list_;

    // Latest navigation state per browser id, pushed from OnLoadingStateChange.
    // Read in OnBeforeContextMenu so the menu's Back/Forward enabled flags
    // reflect the same committed state the click handler will see (CEF's live
    // CanGoBack/CanGoForward can lag right after a navigation kicks off).
    std::map<int, std::pair<bool /*canGoBack*/, bool /*canGoForward*/>> nav_state_;

    // Outstanding context-menu callbacks waiting on a JS-side click. Keyed by
    // an opaque token embedded in the injected page menu; the matching
    // cefQuery from the page carries that token back so we can resolve the
    // correct callback even if the user opens menus across multiple browsers.
    std::map<std::string, CefRefPtr<CefRunContextMenuCallback>> pending_context_menu_callbacks_;
    uint64_t next_context_menu_token_ = 0;

    BrowserCallback browser_created_callback_;

    bool is_closing_;

    BrowserRegisteredFunction browser_registered_function_;

    typedef std::set<CefMessageRouterBrowserSide::Handler *> MessageHandlerSet;
    CefRefPtr<CefMessageRouterBrowserSide> message_router_;
    MessageHandlerSet message_handler_set_;

    // Include the default reference counting implementation.
    IMPLEMENT_REFCOUNTING(TCSimpleHandler);
};

#endif // TESTCEFDEMO_TCSIMPLEHANDLER_H

// ============================================================
// TCSimpleHandler —— CEF 客户端事件处理器（核心）
// 功能：实现 CefClient 及多个 Handler 接口（Display/LifeSpan/
//       Load/Download/Keyboard/Permission/Print/ContextMenu），
//       管理浏览器列表、进程消息通信、导航控制、右键菜单、
//       JS ↔ C++ 双向调用。是 CEF 功能集成的核心类。
// ============================================================

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

// JS 调用 C++ 的回调函数类型
typedef std::function<void(int err_code, const std::string &result)> ReportResultFunction;
typedef std::function<void(const std::string &params, const std::string &func_name, ReportResultFunction callback)>
    CppFunction;
// 浏览器级注册函数映射表
typedef std::map<std::pair<CefString, int>, CppFunction>
    BrowserRegisteredFunction;

class TCSimpleHandler : public CefClient,
                        public CefDisplayHandler,
                        public CefLifeSpanHandler,
                        public CefLoadHandler,
                        public CefDownloadHandler,
                        public CefKeyboardHandler,
                        public CefPermissionHandler,
                        public CefPrintHandler,
                        public CefContextMenuHandler {
public:
    explicit TCSimpleHandler(bool use_views);
    ~TCSimpleHandler();
    static TCSimpleHandler *GetInstance();

    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefDownloadHandler> GetDownloadHandler() override { return this; }
    CefRefPtr<CefKeyboardHandler> GetKeyboardHandler() override { return this; }
    CefRefPtr<CefPermissionHandler> GetPermissionHandler() override { return this; }
    CefRefPtr<CefPrintHandler> GetPrintHandler() override { return this; }
    CefRefPtr<CefContextMenuHandler> GetContextMenuHandler() override { return this; }

    // 右键菜单
    void OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                             CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) override;
    bool RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                        CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
                        CefRefPtr<CefRunContextMenuCallback> callback) override;

    // 显示
    void OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) override;
    void OnFullscreenModeChange(CefRefPtr<CefBrowser> browser, bool fullscreen) override;

    // 生命周期
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, const CefString &target_url,
                       const CefString &target_frame_name,
                       CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture,
                       const CefPopupFeatures &popupFeatures, CefWindowInfo &windowInfo,
                       CefRefPtr<CefClient> &client, CefBrowserSettings &settings,
                       CefRefPtr<CefDictionaryValue> &extra_info, bool *no_javascript_access) override;
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool DoClose(CefRefPtr<CefBrowser> browser) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // 加载
    void OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack,
                              bool canGoForward) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
                     const CefString &errorText, const CefString &failedUrl) override;

    void CloseAllBrowsers(bool force_close);
    bool IsClosing() const { return is_closing_; }
    static bool IsChromeRuntimeEnabled();

    // 下载
    bool CanDownload(CefRefPtr<CefBrowser> browser, const CefString &url,
                     const CefString &request_method) override;
    void OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                          const CefString &suggested_name,
                          CefRefPtr<CefBeforeDownloadCallback> callback) override;
    void OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                           CefRefPtr<CefDownloadItemCallback> callback) override;

    // 键盘
    bool OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event,
                       bool *is_keyboard_shortcut) override;
    bool OnKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event) override { return false; }

    // 打印
    void OnPrintStart(CefRefPtr<CefBrowser> browser) override {}
    void OnPrintSettings(CefRefPtr<CefBrowser> browser, CefRefPtr<CefPrintSettings> settings, bool get_defaults) override {}
    bool OnPrintDialog(CefRefPtr<CefBrowser> browser, bool has_selection, CefRefPtr<CefPrintDialogCallback> callback) override { return false; }
    bool OnPrintJob(CefRefPtr<CefBrowser> browser, const CefString &document_name, const CefString &pdf_file_path, CefRefPtr<CefPrintJobCallback> callback) override { return false; }
    void OnPrintReset(CefRefPtr<CefBrowser> browser) override {}
    CefSize GetPdfPaperSize(CefRefPtr<CefBrowser> browser, int device_units_per_inch) override { return CefSize(); }

    // 权限
    bool OnRequestMediaAccessPermission(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                        const CefString &requesting_origin, uint32 requested_permissions,
                                        CefRefPtr<CefMediaAccessCallback> callback) override { return true; }
    bool OnShowPermissionPrompt(CefRefPtr<CefBrowser> browser, uint64 prompt_id, const CefString &requesting_origin,
                                uint32 requested_permissions, CefRefPtr<CefPermissionPromptCallback> callback) override {
        callback->Continue(CEF_PERMISSION_RESULT_ACCEPT);
        return true;
    }
    void OnDismissPermissionPrompt(CefRefPtr<CefBrowser> browser, uint64 prompt_id,
                                   cef_permission_request_result_t result) override {}

    // JS ↔ C++ 双向通信
    bool OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                  CefProcessId source_process, CefRefPtr<CefProcessMessage> message) override;
    bool RegisterCallBackFunc(const CefString &function_name, CppFunction function, CefRefPtr<CefBrowser> browser, bool replace = false);
    bool ExecuteCallbackFunc(const CefString &function_name, const CefString &params, int js_callback_id,
                             CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
    void CallCPPFunction(const std::string &params, const std::string &func_name, ReportResultFunction callback);

    // 导航回调函数指针 —— 由 qtmodule（mainwindow）启动时设置，桥接到 ArkTS
    typedef void (*NavigateCallback)(const std::string &pageName, int js_callback_id,
                                     CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame);
    static NavigateCallback navigate_callback_;
    static void SetNavigateCallback(NavigateCallback cb) { navigate_callback_ = cb; }

    long getBrowserWindowHandle(int64_t identifier);
    CefRefPtr<CefBrowser> getBrowser(int64_t identifier);
    void LoadURL(int64_t identifier, CefString url);
    void GoBack(int64_t identifier);
    void GoForward(int64_t identifier);
    void DispatchContextMenuChoice(const std::string &token, int command_id);
    int GetBrowserCount() { return browser_list_.size(); }
    using BrowserCallback = std::function<void(int64_t)>;
    bool CreateBrowserForWindow(const long& window, const CefRect& rect, const CefString& url, BrowserCallback callback);

private:
    void PlatformTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title);
    const bool use_views_;

    typedef std::map<long, CefRefPtr<CefBrowser>> BrowserList;
    BrowserList browser_list_;

    std::map<int, std::pair<bool, bool>> nav_state_;

    std::map<std::string, CefRefPtr<CefRunContextMenuCallback>> pending_context_menu_callbacks_;
    uint64_t next_context_menu_token_ = 0;

    BrowserCallback browser_created_callback_;
    bool is_closing_;
    BrowserRegisteredFunction browser_registered_function_;

    typedef std::set<CefMessageRouterBrowserSide::Handler *> MessageHandlerSet;
    CefRefPtr<CefMessageRouterBrowserSide> message_router_;
    MessageHandlerSet message_handler_set_;

    IMPLEMENT_REFCOUNTING(TCSimpleHandler);
};

#endif // TESTCEFDEMO_TCSIMPLEHANDLER_H

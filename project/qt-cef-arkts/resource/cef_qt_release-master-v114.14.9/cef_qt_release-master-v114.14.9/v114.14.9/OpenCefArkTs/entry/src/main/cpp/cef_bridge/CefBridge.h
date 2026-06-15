// ============================================================
// CefBridge.h —— CEF 生命周期管理（纯 CEF + ArkTS 版本）
// 功能：替换原 mainwindow 的 CEF 初始化、桥接注册、浏览器管理角色。
//       在 AbilityStage / EntryAbility 生命周期中被调用。
//       不依赖 QT，适配 XComponent(Surface) 渲染模式。
// ============================================================

#ifndef CEF_BRIDGE_H
#define CEF_BRIDGE_H

#include <string>
#include <thread>
#include <functional>

#include "include/base/cef_logging.h"
#include "include/cef_command_line.h"
#include "include/cef_browser.h"
#include "include/cef_parser.h"

#include "testcef/TCSimpleApp.h"
#include "testcef/TCSimpleHandler.h"
#include "testcef/TCSimpleRender.h"
#include "ohos/adapter_c/adapter_c.h"

class CefBridge {
public:
    CefBridge();
    ~CefBridge();

    // 初始化 CEF：构造命令行参数 → 启动独立线程 → 调用 InitCefLoop
    void StartCef();

    // 在 CEF 初始化完成后创建浏览器实例（由 Surface 就绪后调用）
    // surfaceWindowHandle: XComponent Surface 的原生窗口句柄
    // url: 初始加载地址
    bool CreateBrowser(uint64_t surfaceWindowHandle, const std::string &url);

    // 加载 URL（浏览器已创建时直接加载，否则先创建）
    void LoadURL(const std::string &url);

    // 获取浏览器 ID
    int64_t GetBrowserId() const { return identifier_; }

    // 获取浏览器窗口句柄
    long GetBrowserWindowHandle();

    // 注册桥接回调（替换原 mainwindow 构造函数的角色）
    void RegisterBridges();

    // XComponent Surface 就绪回调（由 ArkTS adapter_c.cefSurfaceReady 调用）
    void OnSurfaceReady(uint64_t surfaceWindowHandle);

    // 关闭 CEF
    void Shutdown();

    // 单例
    static CefBridge *GetInstance();

private:
    // CEF 初始化核心（在独立线程中运行）
    int InitCefLoop(int argc, char *argv[]);

    // 创建浏览器实例（内部调用 handler_->CreateBrowserForWindow）
    bool CreateBrowserInternal(const std::string &url);

    // 桥接回调：CEF JS navigateToNative → ArkTS
    static void OnNavigateCallback(const std::string &pageName,
                                   int js_callback_id,
                                   CefRefPtr<CefBrowser> browser,
                                   CefRefPtr<CefFrame> frame);

    // 桥接回调：ArkTS cefLoadUrl → CEF LoadURL
    static void OnLoadUrlBridge(const std::string &url);

    // 浏览器创建完成回调
    void OnBrowserCreated(int64_t identifier);

private:
    TCSimpleHandler *handler_ = nullptr;
    int64_t identifier_ = 0;
    bool cef_instantiated_ = false;
    bool browser_created_ = false;
    bool shutdown_ = false;
    uint64_t surface_handle_ = 0;
    std::string pending_url_;
    std::string default_url_;

    static CefBridge *instance_;

    // CEF 命令行参数（与 Qt 版本保持一致，移除 QT 特定项）
    static const std::vector<std::string> kCefArgs;
};

#endif // CEF_BRIDGE_H

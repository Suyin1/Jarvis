// ============================================================
// CefBridge.cpp —— CEF 生命周期管理实现
// 功能：CEF 初始化（含命令行参数）、浏览器创建/加载、
//       桥接回调注册（替换原 mainwindow 的 SetNavigateCallback/SetLoadUrlBridge）
// ============================================================

#include "CefBridge.h"
#include "include/cef_origin_whitelist.h"

#include <sstream>

using namespace EMBEDDED_WINDOW_ADAPTER;

CefBridge *CefBridge::instance_ = nullptr;

// CEF 命令行参数（与 Qt 版本一致）
// 注意：移除了 "--in-process-gpu" 因为纯 CEF 版本需确认 GPU 进程模式
const std::vector<std::string> CefBridge::kCefArgs = {
    "cef",
    "--use-gl=egl",
    "--enable-features=UseOzonePlatform",
    "--ozone-platform=ohos",
    "--enable-logging",
    "--v=0",
    "--ozone-dump-file=/data/storage/el2/base/cache/",
    "--log-file=/data/storage/el2/base/cache/test.log",
    "--no-zygote",
    "--in-process-gpu",
    "--log-net-log",
    "--bundle-installation-dir=/data/storage/el1/bundle/entry/resources/resfile/",
    "--content-shell-host-window-size=1139x654",
    "--user-data-dir=/data/storage/el2/base/files/",
    "--enable-media-stream=true",
    "--enable-speech-input=true",
    "--enable-print-preview",
    "--force-renderer-accessibility=basic",
    "--lang=zh-CN",
    "--remote-debugging-port=9222",
    "--remote-allow-origins=http://localhost:9222",
};

CefBridge::CefBridge() {
    handler_ = new TCSimpleHandler(false);
    instance_ = this;
}

CefBridge::~CefBridge() {
    Shutdown();
}

CefBridge *CefBridge::GetInstance() {
    if (!instance_) {
        instance_ = new CefBridge();
    }
    return instance_;
}

// 注册桥接回调：替换原 mainwindow 构造函数的角色
void CefBridge::RegisterBridges() {
    // 方向 A: CEF JS → ArkTS（navigateToNative）
    TCSimpleHandler::SetNavigateCallback(
        [](const std::string &pageName, int js_callback_id,
           CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "CefBridge",
                         "NavigateToNative callback: pageName=%{public}s", pageName.c_str());
            // 通过 adapter_c 的 DefaultNavigateBridge 调用 ArkTS 函数
            if (EWAdapterC::navigate_bridge_) {
                EWAdapterC::navigate_bridge_(pageName);
            }
            // 回调 JS：通知渲染进程导航请求已处理
            if (frame && browser) {
                CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("ExecutedCallbackToJS");
                msg->GetArgumentList()->SetInt(0, js_callback_id);
                msg->GetArgumentList()->SetInt(1, 0);
                msg->GetArgumentList()->SetString(2, R"({"code":0,"msg":"ok"})");
                frame->SendProcessMessage(PID_RENDERER, msg);
            }
        });

    // 方向 B: ArkTS → CEF URL 加载
    EWAdapterC::SetLoadUrlBridge(
        [](const std::string &url) {
            OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "CefBridge",
                         "LoadUrlBridge: url=%{public}s", url.c_str());
            CefBridge::GetInstance()->LoadURL(url);
        });
}

// 启动 CEF（在独立线程中初始化）
void CefBridge::StartCef() {
    std::thread cefThread([this]() {
        std::vector<char *> argv_cstr;
        for (const auto &arg : kCefArgs) {
            argv_cstr.push_back(const_cast<char *>(arg.c_str()));
        }
        InitCefLoop(static_cast<int>(argv_cstr.size()), argv_cstr.data());
    });
    cefThread.detach();
}

// CEF 初始化核心逻辑
int CefBridge::InitCefLoop(int argc, char *argv[]) {
    CefMainArgs main_args(argc, argv);

    CefRefPtr<CefCommandLine> command_line = CefCommandLine::CreateCommandLine();
    command_line->InitFromArgv(argc, argv);

    CefRefPtr<CefApp> app;
    TCSimpleCefApp::ProcessType process_type = TCSimpleCefApp::GetProcessType(command_line);
    if (process_type == TCSimpleCefApp::RendererProcess ||
        process_type == TCSimpleCefApp::ZygoteProcess) {
        app = new TCSimpleRender();
    } else {
        app = new TCSimpleApp();
    }

    int exit_code = CefExecuteProcess(main_args, app, nullptr);
    if (exit_code >= 0) {
        return exit_code;
    }

    CefSettings settings;
    TCSimpleApp::PopulateSettings(command_line, settings);

    if (command_line->HasSwitch("enable-chrome-runtime")) {
        settings.chrome_runtime = true;
    }

#if !defined(CEF_USE_SANDBOX)
    settings.no_sandbox = true;
#endif

    bool bret = CefInitialize(main_args, settings, app.get(), nullptr);
    cef_instantiated_ = bret;
    if (bret) {
        // CEF 初始化成功后立即加载默认 URL
        if (!pending_url_.empty()) {
            LoadURL(pending_url_);
        } else {
            LoadURL(default_url_);
        }
    }

    // 进入 CEF 消息循环（阻塞直到 CefQuitMessageLoop 被调用）
    CefRunMessageLoop();

    CefShutdown();
    shutdown_ = true;
    return 0;
}

// Surface 就绪后创建浏览器
bool CefBridge::CreateBrowser(uint64_t surfaceWindowHandle, const std::string &url) {
    surface_handle_ = surfaceWindowHandle;
    if (!handler_) {
        return false;
    }

    // 使用 Surface 窗口句柄创建浏览器，Rect 在纯 CEF 版本中需适配 ArkUI 坐标
    bool result = handler_->CreateBrowserForWindow(
        surfaceWindowHandle,
        CefRect(0, 0, 1139, 654), // 初始尺寸，后续由 ArkUI 布局动态调整
        url,
        [this](int64_t identifier) { OnBrowserCreated(identifier); });

    browser_created_ = result;
    return result;
}

void CefBridge::LoadURL(const std::string &url) {
    if (browser_created_ && handler_) {
        handler_->LoadURL(GetBrowserId(), url);
    } else {
        pending_url_ = url;
        if (cef_instantiated_ && surface_handle_ != 0) {
            CreateBrowser(surface_handle_, url);
        }
    }
}

long CefBridge::GetBrowserWindowHandle() {
    return handler_ ? handler_->getBrowserWindowHandle(GetBrowserId()) : 0;
}

void CefBridge::OnSurfaceReady(uint64_t surfaceWindowHandle) {
    surface_handle_ = surfaceWindowHandle;
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "CefBridge",
                 "OnSurfaceReady: handle=%{public}llu, browser_created=%{public}d",
                 surfaceWindowHandle, browser_created_);
    if (!browser_created_ && cef_instantiated_) {
        std::string url = pending_url_.empty() ? default_url_ : pending_url_;
        CreateBrowser(surface_handle_, url);
    }
}

void CefBridge::OnBrowserCreated(int64_t identifier) {
    if (identifier_ == 0) {
        identifier_ = identifier;
    }
}

void CefBridge::Shutdown() {
    if (cef_instantiated_ && !shutdown_) {
        if (browser_created_ && handler_) {
            EWAdapterC *adapter = EWAdapterC::getInstance();
            if (adapter) {
                adapter->removeNode((Node *)GetBrowserWindowHandle());
            }
        }
        CefQuitMessageLoop();
    }
}

// 样式回调：CEF JS navigateToNative → ArkTS
void CefBridge::OnNavigateCallback(const std::string &pageName,
                                    int js_callback_id,
                                    CefRefPtr<CefBrowser> browser,
                                    CefRefPtr<CefFrame> frame) {
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "CefBridge",
                 "OnNavigateCallback: pageName=%{public}s", pageName.c_str());
    if (EWAdapterC::navigate_bridge_) {
        EWAdapterC::navigate_bridge_(pageName);
    }
    if (frame && browser) {
        CefRefPtr<CefProcessMessage> msg = CefProcessMessage::Create("ExecutedCallbackToJS");
        msg->GetArgumentList()->SetInt(0, js_callback_id);
        msg->GetArgumentList()->SetInt(1, 0);
        msg->GetArgumentList()->SetString(2, R"({"code":0,"msg":"ok"})");
        frame->SendProcessMessage(PID_RENDERER, msg);
    }
}

void CefBridge::OnLoadUrlBridge(const std::string &url) {
    OH_LOG_Print(LOG_APP, LOG_INFO, 0xFF00, "CefBridge",
                 "OnLoadUrlBridge: url=%{public}s", url.c_str());
    CefBridge::GetInstance()->LoadURL(url);
}

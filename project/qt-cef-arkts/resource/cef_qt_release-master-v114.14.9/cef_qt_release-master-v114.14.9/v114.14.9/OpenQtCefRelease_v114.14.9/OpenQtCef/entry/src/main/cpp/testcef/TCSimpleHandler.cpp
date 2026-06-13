//
// Created on 2024/2/4.
//
// Node APIs are not fully supported. To solve the compilation error of the interface cannot be found,
// please include "napi/native_api.h".

#include "TCSimpleHandler.h"
#include <sstream>
#include <string>
#include <vector>

#include "JsCallAPI.h"
#include "include/base/cef_callback.h"
#include "include/cef_app.h"
#include "include/cef_parser.h"
#include "include/cef_path_util.h"
#include "include/views/cef_browser_view.h"
#include "include/views/cef_window.h"
#include "include/wrapper/cef_closure_task.h"
#include "include/wrapper/cef_helpers.h"
#include "include/wrapper/cef_stream_resource_handler.h"
#include "ohos/adapter/window/app_window_adapter.h"
#include "ohos/adapter/xcomponent/adapter/window_adapter.h"

using namespace ohos::adapter::window;
using namespace ohos::adapter::xcomponent;

namespace {

TCSimpleHandler *g_instance = nullptr;

// Returns a data: URI with the specified contents.
std::string GetDataURI(const std::string &data, const std::string &mime_type) {
    return "data:" + mime_type + ";base64," + CefURIEncode(CefBase64Encode(data.data(), data.size()), false).ToString();
}

// Routes cefQuery requests of the form "cef_ctx_menu:<token>:<command_id>"
// from the in-page context menu (injected by RunContextMenu) back to
// TCSimpleHandler. Forward-declared here so OnAfterCreated below can
// instantiate it; the JS template + helpers live further down near
// RunContextMenu.
class ContextMenuQueryHandler : public CefMessageRouterBrowserSide::Handler {
public:
    explicit ContextMenuQueryHandler(TCSimpleHandler *owner) : owner_(owner) {}

    bool OnQuery(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, int64 query_id, const CefString &request,
                 bool persistent, CefRefPtr<Callback> callback) override {
        const std::string req = request.ToString();
        const std::string kPrefix = "cef_ctx_menu:";
        if (req.compare(0, kPrefix.size(), kPrefix) != 0) {
            return false; // not ours
        }
        // Format: cef_ctx_menu:<token>:<cmd_id>
        size_t sep = req.find(':', kPrefix.size());
        if (sep == std::string::npos) {
            callback->Failure(-1, "malformed");
            return true;
        }
        std::string token = req.substr(kPrefix.size(), sep - kPrefix.size());
        int command_id = -1;
        try {
            command_id = std::stoi(req.substr(sep + 1));
        } catch (...) {
            command_id = -1;
        }
        owner_->DispatchContextMenuChoice(token, command_id);
        callback->Success("");
        return true;
    }

private:
    TCSimpleHandler *owner_;
};

} // namespace

TCSimpleHandler::TCSimpleHandler(bool use_views) : use_views_(use_views), is_closing_(false) {
    DCHECK(!g_instance);
    g_instance = this;
}

TCSimpleHandler::~TCSimpleHandler() { g_instance = nullptr; }

// static
TCSimpleHandler *TCSimpleHandler::GetInstance() { return g_instance; }

void TCSimpleHandler::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString &title) {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "CefDisplayHandler call API: " << __FUNCTION__ << ": " << __LINE__ << " browser: " << browser
              << " title: " << title.ToString() << " , use_views_: " << use_views_ << " , browser"
              << browser->GetHost()->GetWindowHandle();

    if (use_views_) {
        // Set the title of the window using the Views framework.
        CefRefPtr<CefBrowserView> browser_view = CefBrowserView::GetForBrowser(browser);
        if (browser_view) {
            CefRefPtr<CefWindow> window = browser_view->GetWindow();
            if (window) {
                window->SetTitle(title);
            }
        }
    } else if (!IsChromeRuntimeEnabled()) {
        // Set the title of the window using platform APIs.
        PlatformTitleChange(browser, title);
    }
}

void TCSimpleHandler::PlatformTitleChange(CefRefPtr<CefBrowser> browser,
                                          const CefString& title) {
    AppWindowAdapter::GetInstance().SetTitle(
        title.ToString(), WindowAdapter::GetInstance().GetWindowWidgetId());
}

void TCSimpleHandler::OnFullscreenModeChange(CefRefPtr<CefBrowser> browser,
                                             bool fullscreen) {
    CEF_REQUIRE_UI_THREAD();
    WindowWidgetType widget = WindowAdapter::GetInstance().GetWindowWidgetId();
    LOG(INFO) << "[FS-DIAG] TCSimpleHandler::OnFullscreenModeChange widget="
              << widget << " fullscreen=" << fullscreen
              << " use_views_=" << use_views_;
    if (fullscreen) {
        AppWindowAdapter::GetInstance().SetFullscreen(widget);
    } else {
        // 退出 HTML5 全屏：回到普通窗口状态。这里没有跟踪进入全屏前是否为
        // maximized，统一退到 Normal；若之前是 maximized 会有微小退化，避免
        // 跨进程跟踪状态机的复杂度，先以可用为主。
        AppWindowAdapter::GetInstance().UnMaximize(widget);
    }
}

bool TCSimpleHandler::OnBeforePopup(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                    const CefString &target_url, const CefString &target_frame_name,
                                    CefLifeSpanHandler::WindowOpenDisposition target_disposition, bool user_gesture,
                                    const CefPopupFeatures &popupFeatures, CefWindowInfo &windowInfo,
                                    CefRefPtr<CefClient> &client, CefBrowserSettings &settings,
                                    CefRefPtr<CefDictionaryValue> &extra_info, bool *no_javascript_access) {
    // Diagnostic: PiP / window.open / target=_blank all go through here. Print
    // enough to tell which is which. target_disposition values come from
    // cef_window_open_disposition_t in cef_types.h; the one we care about is
    // WOD_NEW_PICTURE_IN_PICTURE for B站/各类视频站的画中画.
    const char *disp_name = "?";
    switch (target_disposition) {
    case WOD_UNKNOWN: disp_name = "UNKNOWN"; break;
    case WOD_CURRENT_TAB: disp_name = "CURRENT_TAB"; break;
    case WOD_SINGLETON_TAB: disp_name = "SINGLETON_TAB"; break;
    case WOD_NEW_FOREGROUND_TAB: disp_name = "NEW_FOREGROUND_TAB"; break;
    case WOD_NEW_BACKGROUND_TAB: disp_name = "NEW_BACKGROUND_TAB"; break;
    case WOD_NEW_POPUP: disp_name = "NEW_POPUP"; break;
    case WOD_NEW_WINDOW: disp_name = "NEW_WINDOW"; break;
    case WOD_SAVE_TO_DISK: disp_name = "SAVE_TO_DISK"; break;
    case WOD_OFF_THE_RECORD: disp_name = "OFF_THE_RECORD"; break;
    case WOD_IGNORE_ACTION: disp_name = "IGNORE_ACTION"; break;
    case WOD_SWITCH_TO_TAB: disp_name = "SWITCH_TO_TAB"; break;
    case WOD_NEW_PICTURE_IN_PICTURE: disp_name = "NEW_PICTURE_IN_PICTURE"; break;
    default: disp_name = "OTHER"; break;
    }
    LOG(INFO) << "OnBeforePopup target_disposition=" << static_cast<int>(target_disposition) << " (" << disp_name
              << ") target_url=" << target_url.ToString() << " target_frame_name=" << target_frame_name.ToString()
              << " user_gesture=" << user_gesture << " popupFeatures.width=" << popupFeatures.width
              << " popupFeatures.height=" << popupFeatures.height
              << " windowInfo.windowless_rendering_enabled=" << windowInfo.windowless_rendering_enabled
              << " windowInfo.parent_window=" << windowInfo.parent_window;
    // OpenHarmony has no native popup window backing. Redirect the popup URL into
    // the current frame and cancel the default popup so the user actually sees the page.
    /*if (frame && !target_url.empty()) {
        frame->LoadURL(target_url);
    }
    return true;*/
    return false;
}

void TCSimpleHandler::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    // Diagnostic: when PiP triggers, this should fire for the popup browser
    // with IsPopup()==true. If we see OnBeforePopup but NO matching
    // OnAfterCreated, the OH CEF port is failing to create the popup browser
    // entirely (most likely because windowInfo.parent_window is 0 with no
    // SetAsChild). If we DO see OnAfterCreated but no visible window, the
    // browser was created but has no host widget to render into.
    long window_handle = browser->GetHost() ? browser->GetHost()->GetWindowHandle() : 0;
    LOG(INFO) << "OnAfterCreated id=" << browser->GetIdentifier() << " IsPopup=" << browser->IsPopup()
              << " host_window=" << window_handle;

    // Add to the list of existing browsers.
    int64_t identifier = browser->GetIdentifier();
    browser_list_[identifier] = browser;
    LOG(INFO) << "call API: " << __FUNCTION__ << "  browser_list_: " << browser_list_.size()
              << "  browser: " << browser << " identifier: " << identifier;
    if (browser_created_callback_) {
        browser_created_callback_(identifier);
    }
    int len = sizeof(g_jsFunMap) / sizeof(g_jsFunMap[0]);
    for (auto i = 0; i < len; ++i) {
        RegisterCallBackFunc(g_jsFunMap[i].name,
                             std::bind(&TCSimpleHandler::CallCPPFunction, this, std::placeholders::_1,
                                       std::placeholders::_2, std::placeholders::_3),
                             browser);
    }
    if (!message_router_) {
        // Create the browser-side router for query handling.
        CefMessageRouterConfig config;
        message_router_ = CefMessageRouterBrowserSide::Create(config);

        // Register the context-menu query handler so the JS injected by
        // RunContextMenu can post back via cefQuery. Owned by message_handler_-
        // set_ which is cleaned up in OnBeforeClose.
        auto *ctx_menu_handler = new ContextMenuQueryHandler(this);
        message_handler_set_.insert(ctx_menu_handler);

        // Register handlers with the router.
        MessageHandlerSet::const_iterator it = message_handler_set_.begin();
        for (; it != message_handler_set_.end(); ++it) {
            message_router_->AddHandler(*(it), false);
        }
    }
}

bool TCSimpleHandler::DoClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;

    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed destription of this
    // process.
    if (browser_list_.size() == 1) {
        // Set a flag to indicate that the window close should be allowed.
        is_closing_ = true;
    }

    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void TCSimpleHandler::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    CEF_REQUIRE_UI_THREAD();
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__ << " browser_list_ :" << browser_list_.size();

    // Remove from the list of existing browsers.
    BrowserList::iterator bit = browser_list_.begin();
    while (bit != browser_list_.end()) {
        if (bit->second->IsSame(browser)) {
            bit = browser_list_.erase(bit);
            break;
        } else {
            ++bit;
        }
    }
    nav_state_.erase(browser->GetIdentifier());

    // Cancel any context-menu callbacks still pending for this browser. The
    // token is "<browser_id>_<counter>", so a prefix match catches them all.
    const std::string browser_prefix = std::to_string(browser->GetIdentifier()) + "_";
    for (auto pit = pending_context_menu_callbacks_.begin(); pit != pending_context_menu_callbacks_.end();) {
        if (pit->first.compare(0, browser_prefix.size(), browser_prefix) == 0) {
            pit->second->Cancel();
            pit = pending_context_menu_callbacks_.erase(pit);
        } else {
            ++pit;
        }
    }

    if (browser_list_.empty()) {
        MessageHandlerSet::const_iterator it = message_handler_set_.begin();
        for (; it != message_handler_set_.end(); ++it) {
            message_router_->RemoveHandler(*(it));
            delete *(it);
        }
        message_handler_set_.clear();
        message_router_ = nullptr;
        // All browser windows have closed. Quit the application message loop.
        CefQuitMessageLoop();
    }
}

namespace {

const char kContextMenuJSTemplate[] = R"JS(
(function() {
  var token = @@TOKEN@@;
  var x = @@X@@;
  var y = @@Y@@;
  var items = @@ITEMS@@;

  var existing = document.getElementById('__cef_ctx_menu__');
  if (existing && existing.parentNode) existing.parentNode.removeChild(existing);

  var menu = document.createElement('div');
  menu.id = '__cef_ctx_menu__';
  menu.style.cssText =
    'position:fixed;top:' + y + 'px;left:' + x + 'px;z-index:2147483647;' +
    'background:#fff;border:1px solid #ccc;border-radius:4px;' +
    'box-shadow:2px 2px 8px rgba(0,0,0,0.18);padding:4px 0;' +
    'font:14px sans-serif;min-width:140px;color:#000;' +
    'user-select:none;-webkit-user-select:none;';

  var responded = false;
  function respond(cmdId) {
    if (responded) return;
    responded = true;
    try {
      window.cefQuery({
        request: 'cef_ctx_menu:' + token + ':' + cmdId,
        persistent: false,
        onSuccess: function(){},
        onFailure: function(){}
      });
    } catch (e) {}
    if (menu.parentNode) menu.parentNode.removeChild(menu);
    document.removeEventListener('mousedown', onOutside, true);
    document.removeEventListener('contextmenu', onOutside, true);
    document.removeEventListener('keydown', onKey, true);
  }
  function onOutside(e) {
    if (!menu.contains(e.target)) {
      e.preventDefault();
      respond(-1);
    }
  }
  function onKey(e) {
    if (e.key === 'Escape' || e.keyCode === 27) respond(-1);
  }

  items.forEach(function(item) {
    var node;
    if (item.is_separator) {
      node = document.createElement('hr');
      node.style.cssText = 'margin:4px 0;border:none;border-top:1px solid #eee;';
    } else {
      node = document.createElement('div');
      node.textContent = item.label;
      var enabled = !!item.enabled;
      node.style.cssText =
        'padding:6px 18px;cursor:' + (enabled ? 'pointer' : 'default') + ';' +
        'color:' + (enabled ? '#000' : '#aaa') + ';white-space:nowrap;';
      if (enabled) {
        node.addEventListener('mouseenter', function() { node.style.background = '#e6f0ff'; });
        node.addEventListener('mouseleave', function() { node.style.background = ''; });
        node.addEventListener('click', function() { respond(item.command_id); });
      }
    }
    menu.appendChild(node);
  });

  document.body.appendChild(menu);

  var rect = menu.getBoundingClientRect();
  if (rect.right > window.innerWidth) {
    menu.style.left = Math.max(0, window.innerWidth - rect.width - 4) + 'px';
  }
  if (rect.bottom > window.innerHeight) {
    menu.style.top = Math.max(0, window.innerHeight - rect.height - 4) + 'px';
  }

  setTimeout(function() {
    document.addEventListener('mousedown', onOutside, true);
    document.addEventListener('contextmenu', onOutside, true);
    document.addEventListener('keydown', onKey, true);
  }, 0);
})();
)JS";

std::string ReplaceAll(std::string s, const std::string &from, const std::string &to) {
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

} // namespace

void TCSimpleHandler::DispatchContextMenuChoice(const std::string &token, int command_id) {
    CEF_REQUIRE_UI_THREAD();
    auto it = pending_context_menu_callbacks_.find(token);
    if (it == pending_context_menu_callbacks_.end()) {
        LOG(WARNING) << "DispatchContextMenuChoice: unknown token=" << token << " cmd=" << command_id;
        return;
    }
    CefRefPtr<CefRunContextMenuCallback> cb = it->second;
    pending_context_menu_callbacks_.erase(it);
    LOG(INFO) << "DispatchContextMenuChoice token=" << token << " cmd=" << command_id;
    if (command_id >= 0) {
        cb->Continue(command_id, EVENTFLAG_NONE);
    } else {
        cb->Cancel();
    }
}

bool TCSimpleHandler::RunContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                     CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model,
                                     CefRefPtr<CefRunContextMenuCallback> callback) {
    CEF_REQUIRE_UI_THREAD();
    if (!frame) {
        return false;
    }
    // Authoritative Back/Forward state: union of live CefBrowser and the last
    // OnLoadingStateChange push. We override these onto the item snapshots
    // since the libcef on this port doesn't reliably round-trip SetEnabled
    // through OnBeforeContextMenu.
    bool can_back = browser->CanGoBack();
    bool can_fwd = browser->CanGoForward();
    auto nav_it = nav_state_.find(browser->GetIdentifier());
    if (nav_it != nav_state_.end()) {
        can_back = can_back || nav_it->second.first;
        can_fwd = can_fwd || nav_it->second.second;
    }

    // Build a JSON array of items via CefValue/CefDictionaryValue so the
    // labels are JSON-escaped safely for embedding into the JS template.
    CefRefPtr<CefListValue> items_list = CefListValue::Create();
    size_t count = model->GetCount();
    for (size_t i = 0; i < count; ++i) {
        bool is_separator = (model->GetTypeAt(i) == MENUITEMTYPE_SEPARATOR);
        CefRefPtr<CefDictionaryValue> entry = CefDictionaryValue::Create();
        entry->SetBool("is_separator", is_separator);
        if (!is_separator) {
            int cmd_id = model->GetCommandIdAt(i);
            bool enabled = model->IsEnabledAt(i);
            if (cmd_id == MENU_ID_BACK) {
                enabled = can_back;
            } else if (cmd_id == MENU_ID_FORWARD) {
                enabled = can_fwd;
            }
            entry->SetInt("command_id", cmd_id);
            entry->SetString("label", model->GetLabelAt(i));
            entry->SetBool("enabled", enabled);
        }
        items_list->SetDictionary(items_list->GetSize(),  entry);
    }
    CefRefPtr<CefValue> items_value = CefValue::Create();
    items_value->SetList(items_list);
    std::string items_json = CefWriteJSON(items_value, JSON_WRITER_DEFAULT).ToString();

    // Allocate a token and stash the callback. Token includes the browser id
    // so it's globally unique even across multiple browsers.
    std::ostringstream token_oss;
    token_oss << browser->GetIdentifier() << "_" << ++next_context_menu_token_;
    std::string token = token_oss.str();
    pending_context_menu_callbacks_[token] = callback;

    // Substitute placeholders into the JS template.
    std::string js = kContextMenuJSTemplate;
    js = ReplaceAll(js, "@@TOKEN@@", "'" + token + "'");
    js = ReplaceAll(js, "@@X@@", std::to_string(params->GetXCoord()));
    js = ReplaceAll(js, "@@Y@@", std::to_string(params->GetYCoord()));
    js = ReplaceAll(js, "@@ITEMS@@", items_json);

    LOG(INFO) << "RunContextMenu id=" << browser->GetIdentifier() << " token=" << token << " items=" << count
              << " back=" << can_back << " fwd=" << can_fwd << " at=(" << params->GetXCoord() << ","
              << params->GetYCoord() << ")";

    frame->ExecuteJavaScript(js, frame->GetURL(), 0);
    return true;
}

void TCSimpleHandler::OnLoadingStateChange(CefRefPtr<CefBrowser> browser, bool isLoading, bool canGoBack,
                                           bool canGoForward) {
    CEF_REQUIRE_UI_THREAD();
    if (!browser) {
        return;
    }
    nav_state_[browser->GetIdentifier()] = {canGoBack, canGoForward};
    LOG(INFO) << "OnLoadingStateChange id=" << browser->GetIdentifier() << " isLoading=" << isLoading
              << " canGoBack=" << canGoBack << " canGoForward=" << canGoForward;
}

void TCSimpleHandler::OnBeforeContextMenu(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                          CefRefPtr<CefContextMenuParams> params, CefRefPtr<CefMenuModel> model) {
    CEF_REQUIRE_UI_THREAD();
    if (!browser) {
        return;
    }
    // CEF's default menu builder and the live CanGoBack/CanGoForward calls can
    // each return a stale value relative to the click handler -- producing the
    // grayed-but-actually-clickable Forward symptom. Take the union of (a) the
    // live query and (b) the last state pushed via OnLoadingStateChange so
    // that if any source of truth says navigation is possible, the menu is
    // enabled. CEF's internal MENU_ID_FORWARD handler is a safe no-op if the
    // history truly has no forward entry.
    bool can_back = browser->CanGoBack();
    bool can_fwd = browser->CanGoForward();
    auto it = nav_state_.find(browser->GetIdentifier());
    if (it != nav_state_.end()) {
        can_back = can_back || it->second.first;
        can_fwd = can_fwd || it->second.second;
    }
    LOG(INFO) << "OnBeforeContextMenu id=" << browser->GetIdentifier()
              << " live(b/f)=" << browser->CanGoBack() << "/" << browser->CanGoForward()
              << " cached(b/f)=" << (it != nav_state_.end() ? it->second.first : false) << "/"
              << (it != nav_state_.end() ? it->second.second : false) << " applied(b/f)=" << can_back << "/"
              << can_fwd;
    if (model->GetIndexOf(MENU_ID_BACK) != -1) {
        model->SetEnabled(MENU_ID_BACK, can_back);
    }
    if (model->GetIndexOf(MENU_ID_FORWARD) != -1) {
        model->SetEnabled(MENU_ID_FORWARD, can_fwd);
    }
}

void TCSimpleHandler::OnLoadError(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame, ErrorCode errorCode,
                                  const CefString &errorText, const CefString &failedUrl) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    CEF_REQUIRE_UI_THREAD();

    // Allow Chrome to show the error page.
    if (IsChromeRuntimeEnabled()) {
        return;
    }

    // Don't display an error for downloaded files.
    if (errorCode == ERR_ABORTED) {
        return;
    }

    // Display a load error message using a data: URI.
    std::stringstream ss;
    ss << "<html><body bgcolor=\"white\">"
          "<h2>Failed to load URL "
       << std::string(failedUrl) << " with error " << std::string(errorText) << " (" << errorCode
       << ").</h2></body></html>";

    frame->LoadURL(GetDataURI(ss.str(), "text/html"));
}

void TCSimpleHandler::CloseAllBrowsers(bool force_close) {
    if (!CefCurrentlyOn(TID_UI)) {
        // Execute on the UI thread.
        CefPostTask(TID_UI, base::BindOnce(&TCSimpleHandler::CloseAllBrowsers, this, force_close));
        return;
    }

    if (browser_list_.empty()) {
        return;
    }

    BrowserList::const_iterator it = browser_list_.begin();
    for (; it != browser_list_.end(); ++it) {
        CefRefPtr<CefBrowser> browser = it->second;
        if (browser && browser->GetHost()) {
            browser->GetHost()->CloseBrowser(force_close);
        }
    }
}

// static
bool TCSimpleHandler::IsChromeRuntimeEnabled() {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    static int value = -1;
    if (value == -1) {
        CefRefPtr<CefCommandLine> command_line = CefCommandLine::GetGlobalCommandLine();
        value = command_line->HasSwitch("enable-chrome-runtime") ? 1 : 0;
    }
    return value == 1;
}

bool TCSimpleHandler::CanDownload(CefRefPtr<CefBrowser> browser, const CefString &url,
                                  const CefString &request_method) {
    // Allow the download.
    return true;
}

void TCSimpleHandler::OnBeforeDownload(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                                       const CefString &suggested_name, CefRefPtr<CefBeforeDownloadCallback> callback) {
    CefString download_dir;
    if (CefGetPath(PK_DIR_RESOURCES, download_dir) && !download_dir.empty()) {
        std::string download_path = download_dir.ToString() + "/" + suggested_name.ToString();
        CefString file_path = CefString(download_path);
        callback->Continue(file_path, true);
    } else {
        LOG(INFO) << "get download path error.";
    }
}

void TCSimpleHandler::OnDownloadUpdated(CefRefPtr<CefBrowser> browser, CefRefPtr<CefDownloadItem> download_item,
                                        CefRefPtr<CefDownloadItemCallback> callback) {}

bool TCSimpleHandler::OnPreKeyEvent(CefRefPtr<CefBrowser> browser, const CefKeyEvent &event, CefEventHandle os_event,
                                    bool *is_keyboard_shortcut) {
    LOG(INFO) << "Enter CefKeyboardHandler call API OnPreKeyEvent";
    // Ctrl+1 to Ctrl+5 only for test
    if (browser &&  event.type == KEYEVENT_RAWKEYDOWN) {
        switch (event.windows_key_code) {
        case 0x7B:
            if (browser->GetHost()) {
                CefWindowInfo windowInfo;
                CefBrowserSettings settings;
                browser->GetHost()->ShowDevTools(windowInfo, this, settings,
                                                 CefPoint());
            }
            break;
        default:
            break;
        }
    }
    if (browser && event.modifiers & EVENTFLAG_CONTROL_DOWN) {
        if (event.type == KEYEVENT_KEYUP) {
            switch (event.windows_key_code) {
            case '1': {
                if (browser->CanGoBack()) {
                    browser->GoBack();
                }
            } break;
            case '2': {
                if (browser->CanGoForward()) {
                    browser->GoForward();
                }
            } break;
            case '3':
                browser->Reload();
                break;
            case '4':
                browser->ReloadIgnoreCache();
                break;
            case '5':
                browser->StopLoad();
                break;
            default:
                break;
            }
        }
    }
    return false;
}

bool TCSimpleHandler::OnProcessMessageReceived(CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame,
                                               CefProcessId source_process, CefRefPtr<CefProcessMessage> message) {
    LOG(INFO) << "Enter CefClient call API " << __FUNCTION__;
    if (message_router_->OnProcessMessageReceived(browser, frame, source_process, message)) {
        LOG(INFO) << "Leave CefClient call API OnProcessMessageReceived return value:" << true;
        return true;
    }

    // render send message data
    std::string message_name = message->GetName();
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__ << "  message_name: " << message_name;

    if (message_name == "CallBackFunction") {
        CefRefPtr<CefListValue> input_args = message->GetArgumentList();
        LOG(INFO) << "CallBackFunction: " << __FUNCTION__ << ": " << __LINE__
                  << " input_args.size: " << input_args->GetSize();
        CefString fun_name = message->GetArgumentList()->GetString(0);
        CefString params = message->GetArgumentList()->GetString(1);
        int js_callback_id = message->GetArgumentList()->GetInt(2);
        ExecuteCallbackFunc(fun_name, params, js_callback_id, browser, frame);
        LOG(INFO) << "Leave CefClient call API OnProcessMessageReceived return value:" << true;
        return true;
    }
    return false;
}

bool TCSimpleHandler::RegisterCallBackFunc(const CefString &function_name, CppFunction function,
                                           CefRefPtr<CefBrowser> browser, bool replace) {
    if (replace) {
        browser_registered_function_.emplace(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1),
                                             function);
        return true;
    } else {
        auto it =
            browser_registered_function_.find(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1));
        if (it == browser_registered_function_.cend()) {
            browser_registered_function_.emplace(std::make_pair(function_name, browser ? browser->GetIdentifier() : -1),
                                                 function);
            return true;
        }
    }
    return false;
}

bool TCSimpleHandler::ExecuteCallbackFunc(const CefString &function_name, const CefString &params, int js_callback_id,
                                          CefRefPtr<CefBrowser> browser, CefRefPtr<CefFrame> frame) {
    DCHECK(CefCurrentlyOn(TID_UI));
    // js callback
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__ << " function_name: " << function_name
              << " params: " << params << " js_callback_id: " << js_callback_id << " browser: " << browser
              << " frame: " << frame;
    if (function_name == "showMsg") {
        LOG(INFO) << "call API: " << __FUNCTION__ << "---lxh---------do something here";
        if (frame) {
            std::string text = "alert('Hello, this alert is from js call c++, function_name: ";
            text.append(function_name.ToString());
            text.append(" params: ");
            text.append(params.ToString());
            text.append("');");
            frame->ExecuteJavaScript(CefString(text), frame->GetURL(), 0);
        }
    }
    CefRefPtr<CefProcessMessage> message = CefProcessMessage::Create("ExecutedCallbackToJS");
    CefRefPtr<CefListValue> args = message->GetArgumentList();
    auto it = browser_registered_function_.find(std::make_pair(function_name, browser->GetIdentifier()));
    if (it != browser_registered_function_.cend()) {
        auto function = it->second;
        if (function != nullptr) {
            function(params, function_name.ToString(), [=](int err_code, const std::string &json_result) {
                args->SetInt(0, js_callback_id);
                args->SetInt(1, err_code);
                args->SetString(2, json_result);
                frame->SendProcessMessage(PID_RENDERER, message);
            });
        }
        return true;
    }

    it = browser_registered_function_.find(std::make_pair(function_name, -1));
    if (it != browser_registered_function_.cend()) {
        auto function = it->second;
        function(params, function_name.ToString(), [=](int err_code, const std::string &json_result) {
            args->SetInt(0, js_callback_id);
            args->SetInt(1, err_code);
            args->SetString(2, json_result);
            frame->SendProcessMessage(PID_RENDERER, message);
        });
        return true;
    } else {
        args->SetInt(0, js_callback_id);
        args->SetInt(1, -1);
        args->SetString(2, R"({"message":"Function does not exist."})");
        frame->SendProcessMessage(PID_RENDERER, message);
        return false;
    }
}

void TCSimpleHandler::CallCPPFunction(const std::string &params, const std::string &func_name,
                                      ReportResultFunction callback) {
    LOG(INFO) << "call API: " << __FUNCTION__ << ": " << __LINE__;
    std::string *pStr = new std::string(params);
    ReportResultFunction *pReportResult = new ReportResultFunction;
    if (nullptr == pStr || nullptr == pReportResult) {
        return;
    }

    *pReportResult = std::move(callback);
}

long TCSimpleHandler::getBrowserWindowHandle(int64_t identifier) {
    long result = 0;
    if (!browser_list_.empty() && identifier != 0) {
        BrowserList::const_iterator it = browser_list_.begin();
        for (; it != browser_list_.end(); ++it) {
            CefRefPtr<CefBrowser> browser = it->second;
            if (browser && browser->GetIdentifier() == identifier && browser->GetHost()) {
                result = browser->GetHost()->GetWindowHandle();
                break;
            }
        }
    }
    return result;
}

CefRefPtr<CefBrowser> TCSimpleHandler::getBrowser(int64_t identifier) {
    if (!browser_list_.empty() && identifier != 0) {
        BrowserList::const_iterator it = browser_list_.begin();
        for (; it != browser_list_.end(); ++it) {
            CefRefPtr<CefBrowser> browser = it->second;
            if (browser && browser->GetIdentifier() == identifier && browser->GetHost()) {
                return browser;
            }
        }
    }
    return nullptr;
}

void TCSimpleHandler::LoadURL(int64_t identifier, CefString url) {
    CefRefPtr<CefBrowser> browser = getBrowser(identifier);
    if (browser && browser->GetMainFrame())
        browser->GetMainFrame()->LoadURL(url);
}

void TCSimpleHandler::GoBack(int64_t identifier) {
    CefRefPtr<CefBrowser> browser = getBrowser(identifier);
    if (browser && browser->CanGoBack())
        browser->GoBack();
}

void TCSimpleHandler::GoForward(int64_t identifier) {
    CefRefPtr<CefBrowser> browser = getBrowser(identifier);
    if (browser && browser->CanGoForward())
        browser->GoForward();
}

bool TCSimpleHandler::CreateBrowserForWindow(const long& window, const CefRect& rect, const CefString& url, BrowserCallback callback) {
    browser_created_callback_ = callback;
    CefWindowInfo window_info;
    window_info.SetAsChild(window, rect);
    CefBrowserSettings browser_settings;
    return CefBrowserHost::CreateBrowser(window_info, this, url, browser_settings, nullptr, nullptr);
}
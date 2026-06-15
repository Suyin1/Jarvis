// ============================================================
// adapter_c.h —— ArkUI 原生节点适配器（CEF ↔ ArkUI 桥接）
// 功能：通过 ArkUI NativeNode API 创建/管理 XComponent 节点，
//       实现 CEF 浏览器内容渲染到 ArkUI 表面的能力。
//       提供节点树操作（创建根节点/子节点、移动、缩放、聚焦）。
// ============================================================

// Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

#ifndef EMBEDDED_WINDOW_ADAPTERC_H
#define EMBEDDED_WINDOW_ADAPTERC_H

#include <ace/xcomponent/native_interface_xcomponent.h>
#include <arkui/native_node.h>
#include <arkui/native_type.h>
#include <arkui/native_interface.h>
#include <js_native_api.h>
#include <js_native_api_types.h>
#include <vector>
#include <string>
#include <thread>

namespace EMBEDDED_WINDOW_ADAPTER {

const unsigned int LOG_PRINT_DOMAIN = 0xFF00;

#define TEST_EWNODE

// 节点矩形区域
struct NodeRect {
    int64_t offsetX = 0;
    int64_t offsetY = 0;
    int64_t width = 0;
    int64_t height = 0;
};

// XComponent 组件模型配置
struct XComponentModel {
    std::string id = "";
    ArkUI_XComponentType type = ARKUI_XCOMPONENT_TYPE_SURFACE;
    XComponentModel(std::string id, ArkUI_XComponentType type) : id(id), type(type) {}
};

// 节点属性配置（尺寸、位置、XComponent 模型）
class NodeAttribute {
public:
    NodeAttribute() {}
    NodeAttribute(int _width, int _height, int _x, int _y, XComponentModel *_componentModel)
        : width(_width), height(_height), x(_x), y(_y), componentModel(_componentModel) {}

    ~NodeAttribute() {
        if (componentModel != nullptr) {
            delete componentModel;
            componentModel = nullptr;
        }
    }
    int width = 0;
    int height = 0;
    uint32_t x = 0;
    uint32_t y = 0;
    float widthPercent = 1.0;
    float heightPercent = 1.0;
    XComponentModel *componentModel = nullptr;
};

// 公开的 Node 结构（不可修改）
struct Node {
    ArkUI_NodeType nodeType;      // 节点类型（ARKUI_NODE_XCOMPONENT 等）
    ArkUI_NodeHandle node;        // ArkUI 节点句柄
    ArkUI_NodeHandle container;   // 容器节点（必须是 ARKUI_NODE_STACK）
    char nodeOwner[8];            // 节点所有者标识，如 "QT"、"CEF"、"SDL2"、"FLUT"
    void* nodePrivate;            // 所有者自定义数据结构指针
};

// EWNode —— 嵌入式窗口节点（仅在 CEF 中使用）
class EWNode {
public:
    EWNode();
    EWNode(ArkUI_NodeHandle _stack, ArkUI_NodeHandle _xc, ArkUI_NativeNodeAPI_1 *_nodeApi,
           NodeAttribute *_nodeAttributes);
    ~EWNode();

    ArkUI_NodeHandle getXComponent();
    Node *getParent();
    void setParent(Node *parentNode);
    ArkUI_NodeHandle getStack();
    NodeAttribute *getNodeParams();
    int32_t getZIndex();
    void setZIndex(int32_t cur);

private:
    ArkUI_NodeHandle stack = nullptr;           // Stack 容器句柄
    ArkUI_NodeHandle xcomponent = nullptr;      // XComponent 句柄
    Node *parent = nullptr;                     // 父节点
    NodeAttribute *nodeAttributes = nullptr;    // 节点属性
    ArkUI_NativeNodeAPI_1 *nodeApi = nullptr;   // ArkUI 原生节点 API
    int32_t zIndex = 0;                         // Z 序
};

// EWAdapterC —— ArkUI 原生节点适配器（单例）
// 提供 CEF 窗口在 ArkUI 中的节点树操作：创建、移除、移动、调整大小、聚焦等
class EWAdapterC {
public:
    ~EWAdapterC();
    static EWAdapterC *getInstance();
    static Node *createRootNode(NodeAttribute *node, bool scale = false);
    static Node *addChildNode(Node *parentNode, NodeAttribute *nodeParams, bool scale = false);
    static void changeScaled(float scale);
    static bool removeNode(Node *node);
    static bool raiseNode(Node *node);
    static bool lowerNode(Node *node);
    static bool resizeNode(Node *node, int width, int height);
    static bool reParentNode(Node *node, Node *newParent);
    static bool moveNode(Node *node, int x, int y);
    static bool setNodeVisibility(Node *node, bool showCurNode);
    static bool isVisible(Node *node);
    static bool showNode(Node *node);
    static bool hideNode(Node *node);
    static NodeRect getNodeRect(Node *node);
    void setNativeXComponent(const std::string &id, OH_NativeXComponent *nativeXComponent);
    OH_NativeXComponent *getNativeXComponent(const std::string &id);
    void removeNativeXComponent(const std::string &id);
    void init(napi_env env, napi_value exports);
    std::string getWindowNameByXComponentId(const std::string &id);
    std::string getXComponentParentWindowName(ArkUI_NodeHandle node_handle);
    static bool requestFocus(Node* node);
    static bool loseFocus(Node* node);
    static bool setNodeFocusStatus(Node *node, bool focused);
    static bool isCefNode(Node *node);
    static bool shouldRequestFocus(Node *node);

private:
    EWAdapterC() {}
    static EWAdapterC adapter_c;
    static std::thread::id mainJsThreadId;
    static EWNode *createEWNode(Node *parentNode, NodeAttribute *nodeParamsn, bool scale = false);
    static bool removeEWNode(EWNode *node);
    static bool removeChildNode(EWNode *node);
    static bool removeRootNode(EWNode *node);
    static bool raiseEwNode(EWNode *node);
    static bool lowerEWNode(EWNode *node);
    static bool resizeEWNode(EWNode *node, int width, int height);
    static bool moveEWNode(EWNode *node, int x, int y);
    static bool setEWNodeVisibility(EWNode *node, bool showCurNode);
    static bool getEWNodeVisibility(EWNode *node);
    static bool reparentChildNode(EWNode *node, Node *newParent);
    static float px2vp(int px);

public:
    static ArkUI_NativeNodeAPI_1 *nodeAPI;     // ArkUI 原生节点 API
    static float scaledDensity;                // 屏幕缩放密度
    static napi_env MainEnv;                   // NAPI 主环境
    std::unordered_map<std::string, OH_NativeXComponent *> nativeXComponentMap;
    std::unordered_map<std::string, Node*> nativeNodeMap;

    // 导航回调桥接 — 由 mainwindow 设置，TCSimpleHandler 调用
    // 指向一个 C++ 函数，该函数通过 NAPI 调用 ArkTS 注册的页面跳转函数
    typedef void (*NavigateBridge)(const std::string &pageName);
    static NavigateBridge navigate_bridge_;
    static void SetNavigateBridge(NavigateBridge cb) { navigate_bridge_ = cb; }

    // CEF URL 加载桥接 — 由 mainwindow 设置，adapter_c 的 NAPI 导出调用
    typedef void (*LoadUrlBridge)(const std::string &url);
    static LoadUrlBridge load_url_bridge_;
    static void SetLoadUrlBridge(LoadUrlBridge cb) { load_url_bridge_ = cb; }

    // NAPI 引用 — 供 C++ 侧调用 ArkTS 注册的导航回调函数
    static napi_ref arkts_navigate_ref_;
    // 注册 ArkTS 侧翻页回调（由 NavigationBind.ets 在启动时调用）
    static void RegisterArkTSNavigateCallback(napi_env env, napi_value func);
};

} // namespace EMBEDDED_WINDOW_ADAPTER
#endif // EMBEDDED_WINDOW_ADAPTERC_H

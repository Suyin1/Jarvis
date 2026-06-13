
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

/**
 * NodeRect: Rect of Node
 */
struct NodeRect {
    int64_t offsetX = 0;
    int64_t offsetY = 0;
    int64_t width = 0;
    int64_t height = 0;
};

/**
 * XComponent component attribute configuration
 */
struct XComponentModel {
    std::string id = "";
    ArkUI_XComponentType type = ARKUI_XCOMPONENT_TYPE_SURFACE;
    XComponentModel(std::string id, ArkUI_XComponentType type) : id(id), type(type) {}
};

/**
 * Node attribute configuration
 */
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

// Public definition of the Node structure, which cannot be changed.
struct Node {
    ArkUI_NodeType nodeType; // Type of the node. The value can be ARKUI_NODE_XCOMPONENT or other types supported in later versions.
    ArkUI_NodeHandle node; // ARKUI_NODE_XCOMPONENT type or other.
    ArkUI_NodeHandle container; // Must be of type ARKUI_NODE_STACK.
    char nodeOwner[8]; // Describes the body of the Node structure to be created, such as "QT," "CEF," "SDL2," "FLUT".
    void* nodePrivate; // Structure used for the definition of the nodeOwner body record itself.
};

// EWNode for EmbeddedWindow Node, only defined in cef
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
    ArkUI_NodeHandle stack = nullptr;
    ArkUI_NodeHandle xcomponent = nullptr;
    Node *parent = nullptr;
    NodeAttribute *nodeAttributes = nullptr;
    ArkUI_NativeNodeAPI_1 *nodeApi = nullptr;
    int32_t zIndex = 0;
};

// EWAdapterC only for adjust cef Node
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
    static ArkUI_NativeNodeAPI_1 *nodeAPI;
    static float scaledDensity;
    static napi_env MainEnv;
    std::unordered_map<std::string, OH_NativeXComponent *> nativeXComponentMap;
    std::unordered_map<std::string, Node*> nativeNodeMap;
};

} // namespace EMBEDDED_WINDOW_ADAPTER
#endif // EMBEDDED_WINDOW_ADAPTERC_H

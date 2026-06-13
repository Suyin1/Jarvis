// Copyright (c) Huawei Technologies Co., Ltd. 2024. All rights reserved.

#include "ohos/adapter_c/adapter_c.h"

#include <hilog/log.h>
#include <node_api.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <uv.h>

#define Z_INDEX_MAX 2147483647
#define Z_INDEX_SUBNODE_INIT 65536
#define NODE_OWNER_DEFAULT "cef"

namespace EMBEDDED_WINDOW_ADAPTER {
napi_env EWAdapterC::MainEnv = nullptr;
ArkUI_NativeNodeAPI_1 *EWAdapterC::nodeAPI = nullptr;
EWAdapterC EWAdapterC::adapter_c;
std::thread::id EWAdapterC::mainJsThreadId;
float EWAdapterC::scaledDensity = 1.00;
int zIndex = 100;

// Defines locks and semaphores.
using ThreadLockInfo = struct ThreadLockInfo {
    std::mutex mutex;
    std::condition_variable condition;
    bool ready = false;
};

// Define the parameter structure in the thread.
struct WorkParam {
    EWNode *node = nullptr;
    Node *parentNode = nullptr;
    NodeAttribute *nodeParams = nullptr;
    bool boolStatus = false;
    // return
    EWNode *replyNode = nullptr;
    bool replyStatus = false;
    // lock structure
    std::shared_ptr<ThreadLockInfo> lockInfo;

    WorkParam(EWNode *node, Node *parentNode, NodeAttribute *nodeParams,
        bool boolStatus, std::shared_ptr<ThreadLockInfo> lockInfo) {
        this->node = node;
        this->parentNode = parentNode;
        this->nodeParams = nodeParams;
        this->boolStatus = boolStatus;
        this->lockInfo = lockInfo;
    }

    WorkParam() {}
};

bool GetThreadWorkResult(WorkParam *workParam, uv_after_work_cb uvAfterWork)
{
    if (workParam == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "GetThreadWorkResult: workParam nullptr error");
        return false;
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "RunCallbackType begin");
    uv_loop_s *loop = nullptr;
    napi_get_uv_event_loop(EWAdapterC::MainEnv, &loop);
    if (loop == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "Failed to get_uv_event_loop");
        return false;
    }
    uv_work_t *work = new uv_work_t;
    if (work == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "Failed to new uv_work_t");
        return false;
    }
    work->data = (void *)(workParam);
    uv_work_cb uvWork = [](uv_work_t *work) {};
    uv_queue_work(loop, work, uvWork, uvAfterWork);

    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "childThread lock and wait");
    std::unique_lock<std::mutex> lock(workParam->lockInfo->mutex);
    workParam->lockInfo->condition.wait(lock, [&workParam] {
        return workParam->lockInfo->ready;
    });
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "wait work end");

    delete work;
    return workParam->replyStatus;
}

// just for test
static uint32_t generateRandomHex()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dis(0, 0xFFFFFFFF);
    uint32_t num = dis(gen);
    std::stringstream ss;
    ss << std::hex << std::setw(8) << std::setfill('0') << num;
    std::string hexStr = ss.str();
    return std::stoul(hexStr, nullptr, 16);
}

EWNode::EWNode()
{
}

EWNode::EWNode(ArkUI_NodeHandle _stack, ArkUI_NodeHandle _xc,
               ArkUI_NativeNodeAPI_1 *_nodeApi, NodeAttribute *_nodeAttributes)
    : stack(_stack),
    xcomponent(_xc),
    nodeApi(_nodeApi),
    nodeAttributes(_nodeAttributes)

{
}

EWNode::~EWNode()
{
    if (nodeApi != nullptr) {
        if (stack != nullptr) {
            nodeApi->disposeNode(stack);
        }
        if (xcomponent != nullptr) {
            nodeApi->disposeNode(xcomponent);
        }
    }
}

ArkUI_NodeHandle EWNode::getXComponent()
{
    return this->xcomponent;
}

Node *EWNode::getParent()
{
    return this->parent;
}

void EWNode::setParent(Node *parentNode)
{
    this->parent = parentNode;
}

ArkUI_NodeHandle EWNode::getStack()
{
    return this->stack;
}

NodeAttribute *EWNode::getNodeParams()
{
    return this->nodeAttributes;
}

int32_t EWNode::getZIndex()
{
    return this->zIndex;
}

void EWNode::setZIndex(int32_t cur)
{
    this->zIndex = cur;
}

EWAdapterC::~EWAdapterC()
{
}

float EWAdapterC::px2vp(int px)
{
    return px;
//    if (EWAdapterC::scaledDensity == 0) {
//        OH_LOG_Print(LOG_APP, LOG_WARN, LOG_PRINT_DOMAIN, "EWAdapterC", "scaledDensity error");
//        return px;
//    }
//    return px / EWAdapterC::scaledDensity;
}

EWAdapterC *EWAdapterC::getInstance()
{
    return &EWAdapterC::adapter_c;
}

Node *EWAdapterC::createRootNode(NodeAttribute *nodeParams, bool scale)
{
    EWNode *res = nullptr;
    std::thread::id curThreadId = std::this_thread::get_id();
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "createRootNode mainJsThreadId");
        res = createEWNode(nullptr, nodeParams, scale);
    } else {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "createRootNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(nullptr, nullptr, nodeParams, scale, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyNode = createEWNode(nullptr, param->nodeParams, param->boolStatus);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };
        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyNode;
        delete workParam;
        workParam = nullptr;
    }
    if (res == nullptr) {
      return nullptr;
    }
    struct Node* newNode = (Node *)malloc(sizeof(Node));
    newNode->nodeType = ARKUI_NODE_XCOMPONENT;
    newNode->node = res->getXComponent();
    newNode->container = res->getStack();
    strcpy(newNode->nodeOwner, NODE_OWNER_DEFAULT);
    newNode->nodePrivate = (void*)res;
    std::string id = nodeParams->componentModel->id;
    std::size_t pos = id.find(":");
    if (pos != std::string::npos) {
        id = id.substr(0, pos);
        auto hins = EMBEDDED_WINDOW_ADAPTER::EWAdapterC::getInstance();
        if (hins) {
            auto it = hins->nativeNodeMap.find(id);
            if (it == hins->nativeNodeMap.end()) {
                hins->nativeNodeMap[id] = newNode;
            } else {
                if (hins->nativeNodeMap[id] != newNode) {
                    hins->nativeNodeMap[id] = newNode;
                }
            }
        }
    }
    return newNode;
}

EWNode *EWAdapterC::createEWNode(Node *parentNode, NodeAttribute *nodeParams, bool scale)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode enter");
    ArkUI_NodeHandle parentStack = nullptr;
    int32_t curZINdex = 0;
    if (parentNode != nullptr) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC","createEWNode: parentNode exits");
        parentStack = parentNode->container;
        // default index
        curZINdex = 0;
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC",
            "createEWNode: curZINdex = %{public}d", curZINdex);
    }
    if (nodeParams == nullptr || nodeAPI == nullptr || nodeAPI->createNode == nullptr ||
        nodeAPI->addChild == nullptr || nodeAPI->setAttribute == nullptr || nodeAPI->disposeNode == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode: param nullptr error!");
        return nullptr;
    }

    // create stack
    ArkUI_NodeHandle stack = nodeAPI->createNode(ARKUI_NODE_STACK);
    if (stack == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode: create stack nullptr error!");
        return nullptr;
    }

    nodeAPI->setLengthMetricUnit(stack, ARKUI_LENGTH_METRIC_UNIT_PX);

    int32_t res = ARKUI_ERROR_CODE_NO_ERROR;
    if (scale) {
        // set stack scale percent
        ArkUI_NumberValue widthPercentValue[] = {{.f32 = nodeParams->widthPercent}};
        ArkUI_AttributeItem widthPercentItem = {widthPercentValue, 1};
        res = nodeAPI->setAttribute(stack, NODE_WIDTH_PERCENT, &widthPercentItem);
        if (res != ARKUI_ERROR_CODE_NO_ERROR) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                         "createEWNode: setAttribute NODE_WIDTH_PERCENT failed!");
        }

        ArkUI_NumberValue heightPercentValue[] = {{.f32 = nodeParams->heightPercent}};
        ArkUI_AttributeItem heightPercentItem = {heightPercentValue, 1};
        res = nodeAPI->setAttribute(stack, NODE_HEIGHT_PERCENT, &heightPercentItem);
        if (res != ARKUI_ERROR_CODE_NO_ERROR) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                         "createEWNode: setAttribute NODE_HEIGHT_PERCENT failed!");
        }
    } else {
        // set stack size
        ArkUI_NumberValue widthValue[] = {{.f32 = px2vp(nodeParams->width)}};
        ArkUI_AttributeItem widthItem = {widthValue, 1};
        res = nodeAPI->setAttribute(stack, NODE_WIDTH, &widthItem);
        if (res != ARKUI_ERROR_CODE_NO_ERROR) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                         "createEWNode: setAttribute NODE_WIDTH failed!");
        }

        ArkUI_NumberValue heightValue[] = {{.f32 = px2vp(nodeParams->height)}};
        ArkUI_AttributeItem heightItem = {heightValue, 1};
        res = nodeAPI->setAttribute(stack, NODE_HEIGHT, &heightItem);
        if (res != ARKUI_ERROR_CODE_NO_ERROR) {
            OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                         "createEWNode: setAttribute NODE_HEIGHT failed!");
        }
    }

    // set NODE_STACK_ALIGN_CONTENT
    ArkUI_NumberValue alignContentValue[] = {{.i32 = ARKUI_ALIGNMENT_TOP_START}};
    ArkUI_AttributeItem alignContentItem = {alignContentValue, 1};
    res = nodeAPI->setAttribute(stack, NODE_STACK_ALIGN_CONTENT, &alignContentItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_STACK_ALIGN_CONTENT failed!");
    }

    // set NODE_Z_INDEX
    ArkUI_NumberValue zIndexValue[] = {{.i32 = zIndex}};
    ArkUI_AttributeItem zIndexItem = {zIndexValue, zIndex};
    res = nodeAPI->setAttribute(stack, NODE_Z_INDEX, &zIndexItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_Z_INDEX failed!");
    }

    // set stack position
    ArkUI_NumberValue positionValue[] = {{.f32 = px2vp(nodeParams->x)}, {.f32 = px2vp(nodeParams->y)}};
    ArkUI_AttributeItem positionItem = {positionValue, 2};
    res = nodeAPI->setAttribute(stack, NODE_POSITION, &positionItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_POSITION failed!");
    }

    // create xcomponent
    ArkUI_NodeHandle xc = nodeAPI->createNode(ARKUI_NODE_XCOMPONENT);
    if (xc == nullptr) {
        nodeAPI->disposeNode(stack);
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: createNode type ARKUI_NODE_XCOMPONENT failed!");
        return nullptr;
    }

    // set xcomponent id
    ArkUI_AttributeItem idItem = {.string = nodeParams->componentModel->id.c_str()};
    res = nodeAPI->setAttribute(xc, NODE_ID, &idItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_XCOMPONENT_ID failed!");
    }

    res = nodeAPI->setAttribute(xc, NODE_XCOMPONENT_ID, &idItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_XCOMPONENT_ID failed!");
    }

    // set xcomponent type
    ArkUI_NumberValue typeValue[] = {{.i32 = nodeParams->componentModel->type}};
    ArkUI_AttributeItem typeItem = {typeValue, 1};
    res = nodeAPI->setAttribute(xc, NODE_XCOMPONENT_TYPE, &typeItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_XCOMPONENT_TYPE failed!");
    }
    
    ArkUI_NumberValue XCzIndexValue[] = {{.i32 = zIndex}};
    ArkUI_AttributeItem XCzIndexItem = {XCzIndexValue, zIndex};
    res = nodeAPI->setAttribute(xc, NODE_Z_INDEX, &XCzIndexItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_Z_INDEX failed!");
    }

    // set xcomponent focusable
    ArkUI_NumberValue focusableValue[] = {{.i32 = 1}};
    ArkUI_AttributeItem focusableItem = {focusableValue, 1};
    res = nodeAPI->setAttribute(xc, NODE_FOCUSABLE, &focusableItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_FOCUSABLE failed!");
    }

    // set xcomponent focus on touch
    ArkUI_NumberValue focusOnTouchValue[] = {{.i32 = 1}};
    ArkUI_AttributeItem focusOnTouchItem = {focusOnTouchValue, 1};
    res = nodeAPI->setAttribute(xc, NODE_FOCUS_ON_TOUCH, &focusOnTouchItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_FOCUS_ON_TOUCH failed!");
    }
    
    // set xcomponent default focus
    ArkUI_NumberValue defaultFocusValue[] = {{.i32 = 1}};
    ArkUI_AttributeItem defaultFocusItem = {defaultFocusValue, 1};
    res = nodeAPI->setAttribute(stack, NODE_DEFAULT_FOCUS, &defaultFocusItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_DEFAULT_FOCUS failed!");
    }
    
    // set focus status
    ArkUI_NumberValue focusStatusValue[] = {{.i32 = 1}};
    ArkUI_AttributeItem focusStatusItem = {focusStatusValue, 1};
    res = nodeAPI->setAttribute(xc, NODE_FOCUS_STATUS, &focusStatusItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_FOCUS_STATUS failed!");
    }
    
    // set xcomponent render fit top_left
    ArkUI_NumberValue renderFitValue[] = {{.i32 = 5}};
    ArkUI_AttributeItem renderFitItem = {renderFitValue, 1};
    res = nodeAPI->setAttribute(xc, NODE_RENDER_FIT, &renderFitItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_RENDER_FIT failed!");
    }

#ifdef TEST_EWNODE
    // set background color for test
    ArkUI_NumberValue xcBackgroundColor[] = {{.u32 = 0xFFFFFFFF}};
    ArkUI_AttributeItem xcBackgroundColorItem = {xcBackgroundColor, 1};
    res = nodeAPI->setAttribute(xc, NODE_BACKGROUND_COLOR, &xcBackgroundColorItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "createEWNode: setAttribute NODE_BACKGROUND_COLOR failed!");
    }
#endif

    nodeAPI->addChild(stack, xc);
    EWNode *ewNode = new EWNode(stack, xc, nodeAPI, nodeParams);
    if (ewNode == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "create ewNode error!");
        nodeAPI->disposeNode(stack);
        nodeAPI->disposeNode(xc);
        return nullptr;
    }
    ewNode->setZIndex(zIndex);

    if (parentStack != nullptr) {
        nodeAPI->addChild(parentStack, stack);
    } else {
        OH_LOG_Print(LOG_APP, LOG_WARN, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode: parentStack nullptr");
    }

    if (parentNode != nullptr) {
        ewNode->setParent(parentNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode: no parent");
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "createEWNode end");
    return ewNode;
}

Node *EWAdapterC::addChildNode(Node *parentNode, NodeAttribute *nodeParams, bool scale)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "addChildNode node========= enter");
    EWNode *res = nullptr;
    std::thread::id curThreadId = std::this_thread::get_id();
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "addChildNode mainJsThreadId");
        res = createEWNode(parentNode, nodeParams, scale);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "addChildNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(nullptr, parentNode, nodeParams, scale, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyNode = createEWNode(param->parentNode, param->nodeParams, param->boolStatus);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };

        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyNode;
        delete workParam;
        workParam = nullptr;
    }
    if (res == nullptr) {
      return nullptr;
    }
    struct Node* newNode = (Node *)malloc(sizeof(Node));
    newNode->nodeType = ARKUI_NODE_XCOMPONENT;
    newNode->node = res->getXComponent();
    newNode->container = res->getStack();
    strcpy(newNode->nodeOwner, NODE_OWNER_DEFAULT);
    newNode->nodePrivate = (void*)res;
    std::string id = nodeParams->componentModel->id;
    std::size_t pos = id.find(":");
    if (pos != std::string::npos) {
        id = id.substr(0, pos);
        auto hins = EMBEDDED_WINDOW_ADAPTER::EWAdapterC::getInstance();
        if (hins) {
            auto it = hins->nativeNodeMap.find(id);
            if (it == hins->nativeNodeMap.end()) {
                hins->nativeNodeMap[id] = newNode;
            } else {
                if (hins->nativeNodeMap[id] != newNode) {
                    hins->nativeNodeMap[id] = newNode;
                }
            }
        }
    }
    return newNode;
}

// 初始化XComponent
void EWAdapterC::init(napi_env env, napi_value exports)
{
    EWAdapterC::MainEnv = env;
    EWAdapterC::nodeAPI = reinterpret_cast<ArkUI_NativeNodeAPI_1 *>(
        OH_ArkUI_QueryModuleInterfaceByName(ARKUI_NATIVE_NODE, "ArkUI_NativeNodeAPI_1"));
    mainJsThreadId = std::this_thread::get_id();
}

std::string EWAdapterC::getWindowNameByXComponentId(const std::string &id) {
    auto it = nativeNodeMap.find(id);
    if (it != nativeNodeMap.end()) {
        if (!isCefNode(nativeNodeMap[id])) {
            return "";
        }
        return getXComponentParentWindowName(nativeNodeMap[id]->node);
    }
    return "";
}

std::string EWAdapterC::getXComponentParentWindowName(ArkUI_NodeHandle node_handle) {
    ArkUI_HostWindowInfo* host_window_info = nullptr;
    int32_t window_res = OH_ArkUI_NodeUtils_GetWindowInfo(node_handle, &host_window_info);
    if (window_res == ARKUI_ERROR_CODE_NO_ERROR && host_window_info) {
        const char* window_name = OH_ArkUI_HostWindowInfo_GetName(host_window_info);
        std::string name = window_name;
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC",
            "EWAdapterC::getXComponentParentWindowName name: %{public}s", name.c_str());
        return name;
    }
    return "";
}

// 记录NativeXComponent
void EWAdapterC::setNativeXComponent(const std::string &id, OH_NativeXComponent *nativeXComponent)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "setNativeXComponent: begin!");
    if (nativeXComponent == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
            "setNativeXComponent: nativeXComponent nullptr error");
        return;
    }

    auto it = nativeXComponentMap.find(id);
    if (it == nativeXComponentMap.end()) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "setNativeXComponent: add new");
        nativeXComponentMap[id] = nativeXComponent;
        return;
    }

    if (nativeXComponentMap[id] != nativeXComponent) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "setNativeXComponent: remove old");
        nativeXComponentMap[id] = nativeXComponent;
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "setNativeXComponent: end");
}

OH_NativeXComponent *EWAdapterC::getNativeXComponent(const std::string &id)
{
    return nativeXComponentMap[id];
}

// 移除记录NativeXComponent
void EWAdapterC::removeNativeXComponent(const std::string &id)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeNativeXComponent: begin");
    auto it = nativeXComponentMap.find(id);
    if (it != nativeXComponentMap.end()) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "removeNativeXComponent: remove old");
        nativeXComponentMap.erase(it);
    }
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeNativeXComponent: end");
}

bool EWAdapterC::removeEWNode(EWNode *node)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeEWNode: begin!");
    if (node == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "removeEWNode: node nullptr error");
        return false;
    }

    if (node->getParent() != nullptr) {
        return removeChildNode(node);
    } else {
        return removeRootNode(node);
    }
}

void EWAdapterC::changeScaled(float scale) {
    scaledDensity = scale;
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC",
            "EWAdapterC::changeScaled scaledDensity: %{public}f", scaledDensity);
}

bool EWAdapterC::removeNode(Node *node)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = false;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeNode mainJsThreadId");
        res = removeEWNode(cefNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(cefNode, nullptr, nullptr, false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = removeEWNode(param->node);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };
        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    delete node;
    node = nullptr;
    return res;
}

bool EWAdapterC::removeRootNode(EWNode *node)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "removeRootNode: begin");
    if (node == nullptr || node->getParent() != nullptr ||
        nodeAPI == nullptr || nodeAPI->removeChild == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "removeRootNode param nullptr error");
        return false;
    }
    if (node->getStack() != nullptr && node->getXComponent() != nullptr) {
        nodeAPI->removeChild(node->getStack(), node->getXComponent());
    }
    delete node;
    node = nullptr;
    return true;
}

bool EWAdapterC::removeChildNode(EWNode *node)
{
    if (node == nullptr || node->getParent() == nullptr ||
        nodeAPI == nullptr || nodeAPI->removeChild == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "removeChildNode param nullptr error");
        return false;
    }

    auto stack = node->getStack();
    if (stack != nullptr && node->getXComponent() != nullptr) {
        nodeAPI->removeChild(stack, node->getXComponent());
    }

//    auto parentStack = node->getParent()->container;
//    if (parentStack != nullptr && stack != nullptr) {
//        nodeAPI->removeChild(parentStack, stack);
//    }
    if (stack != nullptr && nodeAPI != nullptr) {
        auto parentStack = nodeAPI->getParent(stack);
        if (parentStack != nullptr) {
            nodeAPI->removeChild(parentStack, stack);
        }
    }
    if (node->getNodeParams() && node->getNodeParams()->componentModel) {
        std::string id = node->getNodeParams()->componentModel->id;
        std::size_t pos = id.find(":");
        if (!id.empty() && pos != std::string::npos) {
            id = id.substr(0, pos);
            auto hins = EMBEDDED_WINDOW_ADAPTER::EWAdapterC::getInstance();
            if (hins) {
                auto it = hins->nativeNodeMap.find(id);
                if (it == hins->nativeNodeMap.end()) {
                    hins->nativeNodeMap.erase(it);
                }
            }
        }
    }
    delete node;
    node = nullptr;
    return true;
}

// NODE_Z_INDEX up
bool EWAdapterC::raiseEwNode(EWNode *node)
{
    if (node == nullptr || node->getParent() == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN,
            "EWAdapterC", "raiseEwNode node nullptr error");
        return false;
    }
    int32_t newZIndex = node->getZIndex();
    if (newZIndex >= Z_INDEX_MAX) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN,
            "EWAdapterC", "raiseEwNode index invalid, index: %{public}d", newZIndex);
        return false;
    }
    if (nodeAPI == nullptr || nodeAPI->setAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "raiseEwNode nodeAPI nullptr error");
        return false;
    }
    zIndex++;
    ArkUI_NumberValue zIndexValue[] = {{.i32 = zIndex}};
    ArkUI_AttributeItem zIndexItem = {zIndexValue, 1};
    nodeAPI->setAttribute(node->getStack(), NODE_Z_INDEX, &zIndexItem);
    node->setZIndex(zIndex);
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN,
        "EWAdapterC", "raiseEwNode newZIndex = %{public}d", zIndex);
    return true;
}

// NODE_Z_INDEX up
bool EWAdapterC::raiseNode(Node *node)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = false;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "raiseNode mainJsThreadId");
        res = raiseEwNode(cefNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "raiseNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(cefNode, nullptr, nullptr, false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = raiseEwNode(param->node);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };
        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

// NODE_Z_INDEX down
bool EWAdapterC::lowerEWNode(EWNode *node)
{
    if (node == nullptr || node->getParent() == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "lowerEWNode node nullptr error");
        return false;
    }
    int32_t newZIndex = node->getZIndex();
    if (newZIndex <= 1) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN,
            "EWAdapterC", "lowerEWNode node index error, index: %{public}d", newZIndex);
        return false;
    }
    if (nodeAPI == nullptr || nodeAPI->setAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "lowerEWNode nodeAPI nullptr error");
        return false;
    }
    zIndex = zIndex - 1;
    ArkUI_NumberValue zIndexValue[] = {{.i32 = zIndex}};
    ArkUI_AttributeItem zIndexItem = {zIndexValue, 1};
    nodeAPI->setAttribute(node->getStack(), NODE_Z_INDEX, &zIndexItem);
    node->setZIndex(zIndex);
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN,
        "EWAdapterC", "lowerEWNode lowerNode = %{public}d", zIndex);
    return true;
}

// NODE_Z_INDEX down
bool EWAdapterC::lowerNode(Node *node)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = false;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "lowerNode mainJsThreadId");
        res = lowerEWNode(cefNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "lowerNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(cefNode, nullptr, nullptr, false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = lowerEWNode(param->node);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };
        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}


// NODE_WIDTH | NODE_HEIGHT
bool EWAdapterC::resizeEWNode(EWNode *node, int width, int height)
{
    if (node == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "resizeEWNode node nullptr error");
        return false;
    }
    if (nodeAPI == nullptr || nodeAPI->setAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "resizeEWNode nodeAPI nullptr error");
        return false;
    }
    ArkUI_NumberValue widthValue[] = {{.f32 = px2vp(width)}};
    ArkUI_AttributeItem widthItem = {widthValue, 1};
    nodeAPI->setAttribute(node->getStack(), NODE_WIDTH, &widthItem);
    ArkUI_NumberValue heightValue[] = {{.f32 = px2vp(height)}};
    ArkUI_AttributeItem heightItem = {heightValue, 1};
    nodeAPI->setAttribute(node->getStack(), NODE_HEIGHT, &heightItem);
    node->getNodeParams()->width = width;
    node->getNodeParams()->height = height;
    return true;
}

// NODE_WIDTH | NODE_HEIGHT
bool EWAdapterC::resizeNode(Node *node, int width, int height)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = false;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "resizeNode mainJsThreadId");
        res = resizeEWNode(cefNode, width, height);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "resizeNode not in mainJsThreadId");
        WorkParam *workParam = new WorkParam(cefNode, nullptr,
            new NodeAttribute(width, height, 0, 0, nullptr), false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = resizeEWNode(param->node, param->nodeParams->width, param->nodeParams->height);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };
        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

bool EWAdapterC::reparentChildNode(EWNode *node, Node *newParent)
{
    if (node == nullptr || newParent == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "reparentChildNode param nullptr error");
        return false;
    }

    if (nodeAPI == nullptr || nodeAPI->removeChild == nullptr || nodeAPI->addChild == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "reparentChildNode nodeAPI nullptr error");
        return false;
    }
    auto stack = node->getStack();
    if (stack == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "reparentChildNode stack nullptr error");
        return false;
    }
    auto parentNode = node->getParent();
    if (parentNode != nullptr && parentNode->container != nullptr) {
        nodeAPI->removeChild(parentNode->container, stack);
    } else {
        OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "reparentChildNode do not need removeChild");
    }
    auto newParentStack = newParent->container;
    if (newParentStack != nullptr) {
        nodeAPI->addChild(newParentStack, stack);
    } else {
        OH_LOG_Print(LOG_APP, LOG_WARN, LOG_PRINT_DOMAIN, "EWAdapterC", "reparentChildNode addChild error");
    }
    node->setParent(newParent);
    return true;
}

// move Node from ParentA to ParnetB
bool EWAdapterC::reParentNode(Node *node, Node *newParent)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = true;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "reParentNode mainJsThreadId");
        res = reparentChildNode(cefNode, newParent);
    } else {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "reParentNode not in mainJsThreadId");
        WorkParam *workParam =
            new WorkParam(cefNode, newParent, nullptr, false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = reparentChildNode(param->node, param->parentNode);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };

        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

NodeRect EWAdapterC::getNodeRect(Node *node)
{
    NodeRect rect = {-1, -1, -1, -1};
     if (node == nullptr) {
         OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "getNodeRect node nullptr");
         return rect;
     }
    EWNode* cefNode = (EWNode*)node->nodePrivate;
//     if (cefNode == nullptr || cefNode->getXComponent() == nullptr) {
//         OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "getNodeRect cefnode nullptr error");
//         return rect;
//     }
//
//     ArkUI_IntSize size = {0, 0};
//     OH_ArkUI_NodeUtils_GetLayoutSize(cefNode->getXComponent(), &size);
//     rect.width = size.width;
//     rect.height = size.height;
//
//     ArkUI_IntOffset pos = {0, 0};
//     OH_ArkUI_NodeUtils_GetLayoutPositionInWindow(cefNode->getXComponent(), &pos);
//     rect.offsetX = pos.x;
//     rect.offsetY = pos.y;
    ArkUI_IntOffset screenPos = {0, 0};
    OH_ArkUI_NodeUtils_GetLayoutPositionInScreen(cefNode->getXComponent(), &screenPos);
    OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "djh", "screenx: %{public}d  screen y: %{public}d", screenPos.x, screenPos.y);
    ArkUI_IntOffset translateScreenPos = {0, 0};
    OH_ArkUI_NodeUtils_GetPositionWithTranslateInScreen(cefNode->getXComponent(), &translateScreenPos);
    OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "djh", "translateScreenPos: %{public}d  translateScreenPos y: %{public}d", translateScreenPos.x, translateScreenPos.y);

//     if (cefNode->getParent() != nullptr && cefNode->getParent()->node != nullptr) {
//         ArkUI_IntOffset parentPos = {0, 0};
//         OH_ArkUI_NodeUtils_GetPositionWithTranslateInWindow(cefNode->getParent()->node, &parentPos);
//         rect.offsetX = pos.x - parentPos.x;
//         rect.offsetY = pos.y - parentPos.y;
//     }
    return rect;
}

bool EWAdapterC::moveEWNode(EWNode *node, int x, int y)
{
    if (node == nullptr || node->getStack() == nullptr ||
        nodeAPI == nullptr || nodeAPI->setAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "moveEWNode param nullptr error");
        return false;
    }
    // set stack position
    ArkUI_NumberValue positionValue[] = {{.f32 = px2vp(x)}, {.f32 = px2vp(y)}};
    ArkUI_AttributeItem positionItem = {positionValue, 2};
    int res = nodeAPI->setAttribute(node->getStack(), NODE_POSITION, &positionItem);
    node->getNodeParams()->x = x;
    node->getNodeParams()->y = y;
    return res == ARKUI_ERROR_CODE_NO_ERROR;
}

bool EWAdapterC::moveNode(Node *node, int x, int y)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = true;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "moveNode mainJsThreadId");
        res = moveEWNode(cefNode, x, y);
    } else {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "moveNode not in mainJsThreadId");
        WorkParam *workParam = new WorkParam(cefNode, nullptr,
            new NodeAttribute(0, 0, x, y, nullptr), false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = moveEWNode(param->node, param->nodeParams->x, param->nodeParams->y);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };

        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

bool EWAdapterC::setEWNodeVisibility(EWNode *node, bool showCurNode)
{
    OH_LOG_Print(LOG_APP, LOG_INFO, LOG_PRINT_DOMAIN, "EWAdapterC", "setEWNodeVisibility: %{public}d", showCurNode);
    if (node == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "setEWNodeVisibility node nullptr error");
        return false;
    }
    if (nodeAPI == nullptr || nodeAPI->setAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "setEWNodeVisibility nodeAPI nullptr error");
        return false;
    }
    ArkUI_NumberValue value[] = {ARKUI_VISIBILITY_VISIBLE};
    ArkUI_AttributeItem item = {value, 1};
    int res = 0;
    if (showCurNode) {
        res = nodeAPI->setAttribute(node->getStack(), NODE_VISIBILITY, &item);
    } else {
        value[0].i32 = ARKUI_VISIBILITY_HIDDEN;
        res = nodeAPI->setAttribute(node->getStack(), NODE_VISIBILITY, &item);
    }
    return res == ARKUI_ERROR_CODE_NO_ERROR;
}

bool EWAdapterC::setNodeVisibility(Node *node, bool showCurNode)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = true;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "setNodeVisibility mainJsThreadId");
        res = setEWNodeVisibility(cefNode, showCurNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "setNodeVisibility not in mainJsThreadId");
        WorkParam *workParam = new WorkParam(cefNode, nullptr, nullptr, showCurNode, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = setEWNodeVisibility(param->node, param->boolStatus);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };

        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

bool EWAdapterC::getEWNodeVisibility(EWNode *node)
{
    if (node == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "getEWNodeVisibility node nullptr error");
        return false;
    }
    if (nodeAPI == nullptr || nodeAPI->getAttribute == nullptr) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "getEWNodeVisibility nodeAPI nullptr error");
        return false;
    }

    const ArkUI_AttributeItem *item = nodeAPI->getAttribute(node->getStack(), NODE_VISIBILITY);
    if (item->value[0].i32 == ARKUI_VISIBILITY_VISIBLE) {
        return true;
    }
    return false;
}

bool EWAdapterC::isVisible(Node *node)
{
    if (!isCefNode(node)) {
        return false;
    }
    bool res = true;
    std::thread::id curThreadId = std::this_thread::get_id();
    EWNode* cefNode = (EWNode*)node->nodePrivate;
    if (curThreadId == mainJsThreadId) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "isVisible mainJsThreadId");
        res = getEWNodeVisibility(cefNode);
    } else {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, LOG_PRINT_DOMAIN, "EWAdapterC", "isVisible not in mainJsThreadId");
        WorkParam *workParam = new WorkParam(cefNode, nullptr, nullptr, false, std::make_shared<ThreadLockInfo>());
        uv_after_work_cb afterFunc = [](uv_work_t *work, int _status) {
            WorkParam *param = (WorkParam *)(work->data);
            param->replyStatus = getEWNodeVisibility(param->node);
            param->lockInfo->ready = true;
            param->lockInfo->condition.notify_all();
        };

        GetThreadWorkResult(workParam, afterFunc);
        res = workParam->replyStatus;
        delete workParam;
        workParam = nullptr;
    }
    return res;
}

bool EWAdapterC::showNode(Node *node)
{
    return setNodeVisibility(node, true);
}
bool EWAdapterC::hideNode(Node *node)
{
    return setNodeVisibility(node, false);
}

bool EWAdapterC::requestFocus(Node *node) {
    if (!shouldRequestFocus(node)) {
        return false;
    }
    return setNodeFocusStatus(node, true);
}

bool EWAdapterC::loseFocus(Node *node) {
    return setNodeFocusStatus(node, false);
}

bool EWAdapterC::setNodeFocusStatus(Node *node, bool focused) {
    if (!isCefNode(node)) {
        return false;
    }
    EWNode *child = (EWNode *)node->nodePrivate;
    if (!child || !child->getXComponent()) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC", "loseFocus child nullptr");
        return false;
    }
    ArkUI_NumberValue focusStatusValue[] = {{.i32 = focused ? 1 : 0}};
    ArkUI_AttributeItem focusStatusItem = {
        .value = focusStatusValue,
        .size = 1
    };
    int res = nodeAPI->setAttribute(child->getXComponent(), NODE_FOCUS_STATUS, &focusStatusItem);
    if (res != ARKUI_ERROR_CODE_NO_ERROR) {
        OH_LOG_Print(LOG_APP, LOG_ERROR, LOG_PRINT_DOMAIN, "EWAdapterC",
                     "Failed to setNodeFocusStatus, res: %{public}d", res);
        return false;
    }
    return true;
}

bool EWAdapterC::isCefNode(Node *node) {
    return node &&
           node->node &&
           node->nodeOwner &&
           strcmp(node->nodeOwner, NODE_OWNER_DEFAULT) == 0 &&
           node->nodeType == ARKUI_NODE_XCOMPONENT;
}

bool EWAdapterC::shouldRequestFocus(Node *node) {
    return isVisible(node);
}

} // namespace EMBEDDED_WINDOW_ADAPTER

static napi_value InitFunc(napi_env env, napi_callback_info info) {
    OH_LOG_Print(LOG_APP, LOG_DEBUG, 0, "EWAdapterC", "djh init func enter");
    size_t argc = 1;
    napi_value args[1] = {nullptr};

    if (napi_get_cb_info(env, info, &argc, args, nullptr, nullptr) != napi_ok) {
        napi_throw_type_error(env, nullptr, "napi_get_cb_info failed");
        return nullptr;
    }
    double process_type = -1.0;
    if (napi_get_value_double(env, args[0], &process_type) != napi_ok) {
        napi_throw_type_error(env, nullptr, "napi_get_value failed");
        return nullptr;
    }
    EMBEDDED_WINDOW_ADAPTER::EWAdapterC::scaledDensity = process_type;
    return nullptr;
}

static napi_value GetWindowNameByXComponentId(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[1];
    napi_get_cb_info(env, info, &argc, args, nullptr, nullptr);
    napi_valuetype valuetype;
    napi_typeof(env, args[0], &valuetype);
    if (valuetype != napi_string) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, 0, "EWAdapterC", "Argument must be a string (id).");
        return nullptr;
    }
    char id[256];
    size_t id_length;
    napi_get_value_string_utf8(env, args[0], id, sizeof(id), &id_length);
    napi_value result;
    std::string window_name = EMBEDDED_WINDOW_ADAPTER::EWAdapterC::getInstance()->getWindowNameByXComponentId(id);
    napi_status status = napi_create_string_utf8(env, window_name.data(), window_name.size(), &result);
    if (status != napi_ok) {
        OH_LOG_Print(LOG_APP, LOG_DEBUG, 0, "EWAdapterC", "Failed to create external string.");
        return nullptr;
    }
    return result;
}

EXTERN_C_START static napi_value InitAdapter(napi_env env, napi_value exports) {
  if (env == nullptr || exports == nullptr) {
    return exports;
  }
  napi_property_descriptor desc[] = {
        {"initFunc", 0, InitFunc, 0, 0, 0, napi_default, 0},
        {"getWindowNameByXComponentId", 0, GetWindowNameByXComponentId, 0, 0, 0, napi_default, 0}};
  napi_define_properties(env, exports, sizeof(desc) / sizeof(desc[0]), desc);
  EMBEDDED_WINDOW_ADAPTER::EWAdapterC::getInstance()->init(env, exports);
  return exports;
}

EXTERN_C_END

static napi_module nodeModule = {
    .nm_version = 1,
    .nm_flags = 0,
    .nm_filename = nullptr,
    .nm_register_func = InitAdapter,
    .nm_modname = "adapter_c",
    .nm_priv = ((void *)0),
    .reserved = {0},
};

extern "C" __attribute__((constructor)) void RegisterModule(void) { napi_module_register(&nodeModule); }
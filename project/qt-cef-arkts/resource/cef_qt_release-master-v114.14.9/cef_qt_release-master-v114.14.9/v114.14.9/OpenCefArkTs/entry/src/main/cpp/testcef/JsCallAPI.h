// ============================================================
// JsCallAPI.h —— JS 调用 C++ 接口注册表
// 功能：注册可供 CEF JS 调用的 C++ 函数映射表。
//       当前仅包含 showMsg 演示函数，可扩展。
// ============================================================

#ifndef CEF_JS_CALL_API_H_
#define CEF_JS_CALL_API_H_

#include <string>

namespace {
struct JsIdFunc {
  size_t id;
  std::string name;
};
const JsIdFunc g_jsFunMap[] = {
    {100001, "showMsg"},
    {100002, "navigateToNative"},
};
}  // namespace

#endif  // CEF_JS_CALL_API_H_

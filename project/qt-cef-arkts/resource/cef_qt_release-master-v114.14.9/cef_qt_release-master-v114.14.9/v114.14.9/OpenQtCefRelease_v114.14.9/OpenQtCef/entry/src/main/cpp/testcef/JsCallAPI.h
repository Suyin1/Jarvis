// Copyright (c) 2025 Huawei Device Co., Ltd. All rights reserved
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CEF_JS_CALL_API_H_
#define CEF_JS_CALL_API_H_

#include <string>

// demo for test JS Functions
namespace {
struct JsIdFunc {
  size_t id;
  std::string name;
};
const JsIdFunc g_jsFunMap[] = {
    {100001, "showMsg"},
    // add more js func here.
};
}  // namespace

#endif  // CEF_JS_CALL_API_H_

// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 上下文适配器 JsBinding — 注册 Context 相关函数到 C++，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import { ContextAdapter } from '../adapter/ContextAdapter';


function implContextAdapter(): ContextAdapter {
  return Inject.get(ContextAdapter);
}

function setActiveWindow(id: string) {
  implContextAdapter().setActiveWindow(id);
}

export class ContextAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("ContextAdapter.SetActiveWindow", setActiveWindow);
  }
}
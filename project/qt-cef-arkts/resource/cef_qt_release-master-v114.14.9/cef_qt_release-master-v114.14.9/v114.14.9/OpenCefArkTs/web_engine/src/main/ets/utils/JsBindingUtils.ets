// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// JsBinding 工具类 — 桥接 ArkTS 与原生 C++ 层 (libadapter.so)
// 初始化 NativeContext，注册 JS 函数到 C++ 供 CEF 调用
// 管理主进程/渲染进程上下文切换

import adapter from 'libadapter.so';
import { ContextType } from '../common/Constants';
import { NativeContext } from '../interface/CommonInterface';

export default class JsBindingUtils {
  private static currentContext: NativeContext;

  private static currentType: ContextType;

  static initNativeContext(contextType: ContextType) {
    JsBindingUtils.currentType = contextType;
    JsBindingUtils.currentContext = adapter.getNativeContext(contextType);
  }

  static bindFunction(name: string, func: Function) {
    if (JsBindingUtils.currentContext === undefined) {
      JsBindingUtils.initNativeContext(ContextType.kMainProcess);
    }
    JsBindingUtils.currentContext.JSBind.bindFunction(name, func);
  }

  static callFunction(name: string, ...args: object[]) {
  }

  static getNativeContext(contextType: number): NativeContext {
    if (JsBindingUtils.currentType === contextType && JsBindingUtils.currentContext !== undefined) {
      return JsBindingUtils.currentContext;
    }
    return adapter.getNativeContext(contextType);
  }
}
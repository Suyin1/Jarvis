// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 无障碍适配器 JsBinding — 注册无障碍服务函数到 C++ JSBind，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { AccessibilityAdapter } from '../adapter/AccessibilityAdapter';


function implAccessibilityAdapter(): AccessibilityAdapter {
  return Inject.getOrCreate(AccessibilityAdapter);
}

function speech(text: string, id: string) {
  implAccessibilityAdapter().speech(text, id);
}

function shutDown() {
  implAccessibilityAdapter().shutDown();
}

export class AccessibilityAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("AccessibilityAdapter.Speech", speech);
    JsBindingUtils.bindFunction("AccessibilityAdapter.ShutDown", shutDown);
  }
}

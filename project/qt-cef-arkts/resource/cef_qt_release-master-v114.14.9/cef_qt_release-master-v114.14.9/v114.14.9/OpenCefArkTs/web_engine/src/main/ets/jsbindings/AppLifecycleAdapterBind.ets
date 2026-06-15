// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 应用生命周期适配器 JsBinding — 注册前后台切换回调到 C++，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { AppLifecycleAdapter } from '../adapter/AppLifecycleAdapter';


function implAppLifecycleAdapter(): AppLifecycleAdapter {
  return Inject.getOrCreate(AppLifecycleAdapter);
}

function onWebCreate() {
  implAppLifecycleAdapter().onWebCreate();
}

function onWebDestroy() {
  implAppLifecycleAdapter().onWebDestroy();
}

export class AppLifecycleAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("AppLifecycleAdapter.onWebCreate", onWebCreate);
    JsBindingUtils.bindFunction("AppLifecycleAdapter.onWebDestroy", onWebDestroy);
  }
}

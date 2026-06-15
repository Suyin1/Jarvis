// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 原生主题适配器 JsBinding — 注册深色模式切换回调到 C++

import ConfigurationConstant from '@ohos.app.ability.ConfigurationConstant';
import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { NativeThemeAdapter } from '../adapter/NativeThemeAdapter';

function implNativeThemeAdapter(): NativeThemeAdapter {
  return Inject.getOrCreate(NativeThemeAdapter);
}

function getSystemNativeTheme(): ConfigurationConstant.ColorMode {
  return implNativeThemeAdapter().getSystemNativeTheme();
}

export class NativeThemeAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("NativeThemeAdapter.GetSystemNativeTheme", getSystemNativeTheme);
  }
}

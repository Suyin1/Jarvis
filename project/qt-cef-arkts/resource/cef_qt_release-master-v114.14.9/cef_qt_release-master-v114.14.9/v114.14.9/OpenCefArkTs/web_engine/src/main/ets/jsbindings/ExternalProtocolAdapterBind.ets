// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 外部协议适配器 JsBinding — 注册自定义协议处理函数到 C++，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { ExternalProtocolAdapter } from '../adapter/ExternalProtocolAdapter';

function implExternalProtocolAdapter(): ExternalProtocolAdapter {
  return Inject.getOrCreate(ExternalProtocolAdapter);
}

function openExternal(urlStr: string) {
  implExternalProtocolAdapter().openExternal(urlStr);
}

export class ExternalProtocolAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("ExternalProtocolAdapter.OpenExternal", openExternal);
  }
}

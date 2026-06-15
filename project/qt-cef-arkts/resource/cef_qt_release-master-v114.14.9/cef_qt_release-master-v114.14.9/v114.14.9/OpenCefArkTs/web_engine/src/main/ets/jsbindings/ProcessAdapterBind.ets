// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 进程适配器 JsBinding — 注册进程信息查询函数到 C++

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { ProcessAdapter } from '../adapter/ProcessAdapter';

function implProcessAdapter(): ProcessAdapter {
  return Inject.getOrCreate(ProcessAdapter);
}

function startChildProcess(callback: Function): void {
  implProcessAdapter().startChildProcess(callback);
}

export class ProcessAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("ChromiumChildProcessStarter.StartChildProcess", startChildProcess);
  }
}
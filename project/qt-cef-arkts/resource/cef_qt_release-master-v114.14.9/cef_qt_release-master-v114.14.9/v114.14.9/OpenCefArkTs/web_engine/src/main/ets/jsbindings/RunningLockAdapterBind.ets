// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 运行锁适配器 JsBinding — 注册防止休眠函数到 C++

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { RunningLockAdapter } from '../adapter/RunningLockAdapter';

function implRunningLockAdapter(): RunningLockAdapter {
  return Inject.getOrCreate(RunningLockAdapter);
}

function start() {
  implRunningLockAdapter().start();
}

function stop() {
  implRunningLockAdapter().stop();
}

export class RunningLockAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("RunningLock.Start", start);
    JsBindingUtils.bindFunction("RunningLock.Stop", stop);
  }
}
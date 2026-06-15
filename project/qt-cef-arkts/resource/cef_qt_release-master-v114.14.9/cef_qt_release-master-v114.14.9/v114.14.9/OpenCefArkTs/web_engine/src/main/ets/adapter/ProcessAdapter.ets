// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 进程适配器 — 封装 @ohos.process API，提供进程 ID/名称等信息

import { injectable } from 'inversify';
import childProcessManager from '@ohos.app.ability.childProcessManager';
import { BusinessError } from '@ohos.base';

import { BaseAdapter } from '../common/BaseAdapter';
import LogUtil from '../utils/LogUtil';
import LogMethod from '../common/LogDecorator';

function fireProcessBind(callback: Function) {
  const invalidPid: number = -1;
  try {
    childProcessManager.startChildProcess('./ets/process/CustomChildProcess.ets',
      childProcessManager.StartMode.APP_SPAWN_FORK)
      .then((result) => {
        callback(result);
      }, (err: BusinessError) => {
        callback(invalidPid);
        LogUtil.error('ProcessAdapter', `startChildProcess failed: ${err}`);
      });
  } catch (err) {
    callback(invalidPid);
    LogUtil.error('ProcessAdapter', `startChildProcess failed: ${err}`);
  }
}

@injectable()
export class ProcessAdapter extends BaseAdapter {
  @LogMethod
  startChildProcess(callback: Function): void {
    fireProcessBind(callback);
  }
}
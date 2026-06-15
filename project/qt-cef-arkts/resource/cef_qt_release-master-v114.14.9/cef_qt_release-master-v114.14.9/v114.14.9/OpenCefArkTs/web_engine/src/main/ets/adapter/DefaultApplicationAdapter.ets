// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 默认应用适配器 — 查询和打开系统默认应用（浏览器/邮件等）
// 继承 BaseAdapter，封装 @ohos.bundle.defaultAppManager 和 Want API

import defaultAppMgr from '@ohos.bundle.defaultAppManager';
import { Want } from '@kit.AbilityKit';
import { inject, injectable } from 'inversify';
import { BaseAdapter } from '../common/BaseAdapter';
import LogMethod from '../common/LogDecorator';
import LogUtil from '../utils/LogUtil';
import LaunchHelper from '../utils/LaunchHelper';
import { ContextAdapter } from './ContextAdapter';

const TAG: string = 'DefaultApplicationAdapter';

@injectable()
export class DefaultApplicationAdapter extends BaseAdapter {
  protected ctxAdapter: ContextAdapter;

  constructor(@inject(ContextAdapter) ctxAdapter: ContextAdapter) {
    super();
    this.ctxAdapter = ctxAdapter;
  }

  @LogMethod
  isDefaultApplication(): boolean {
    try {
      return defaultAppMgr.isDefaultApplicationSync(defaultAppMgr.ApplicationType.BROWSER);
    } catch (error) {
      LogUtil.error(TAG, 'Operation failed. Cause: ' + JSON.stringify(error));
      return false;
    }
  }

  @LogMethod
  startSettingsAbility(): boolean {
    const want: Want = {
      bundleName: 'com.huawei.hmos.settings',
      abilityName: 'com.huawei.hmos.settings.MainAbility',
      uri: 'default_browser_settings',
    };
    return LaunchHelper.Launch(this.ctxAdapter.getActiveContext(), want);
  }
}

// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Ability 启动辅助工具 — 封装 startAbility 调用
// 支持带窗口选项 (LaunchWithOptions) 和不带选项 (Launch) 两种模式

import { StartOptions, Want } from '@kit.AbilityKit';
import LogUtil from './LogUtil';
import common from '@ohos.app.ability.common';
import { BusinessError } from '@kit.BasicServicesKit';

const TAG: string = 'LaunchHelper';

export default class LaunchHelper {
  static LaunchWithOptions(uiContext: Context | undefined,
                           want: Want,
                           options: StartOptions): boolean {
    try {
      let context = uiContext as common.UIAbilityContext ||
        uiContext as common.UIExtensionContext;
      if (!context) {
        LogUtil.error(TAG, `excute failed, want: ${JSON.stringify(want)}, options:  ${JSON.stringify(options)}`);
        return false;
      }
      context.startAbility(want, options)
        .catch((error: BusinessError) => {
        LogUtil.error(TAG, `excute failed. Cause: ${JSON.stringify(error)}`);
        return false;
      });
    } catch (exception) {
      LogUtil.error(TAG, `excute failed exception. Cause: ${JSON.stringify(exception)}`);
      return false;
    }
    return true;
  }

  static Launch(uiContext: Context | undefined, want: Want): boolean {
    return LaunchHelper.LaunchWithOptions(uiContext, want, {});
  }
}
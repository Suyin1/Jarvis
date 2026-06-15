// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ============================================================
// AbilityStage —— 应用级生命周期入口
// 功能：初始化 CEF 适配层 (adapter_c)、关联 QPA (Qt Platform Abstraction)
//       平台插件，并管理多窗口的 Ability 实例键分发。
// ============================================================

import adapter_c from 'libadapter_c.so';
import { WebAbilityStage } from 'web_engine';
import display from '@ohos.display';
import { AbilityStage } from '@kit.AbilityKit';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';

// 获取屏幕缩放密度，用于 CEF 适配层的 DPI 缩放初始化
let scaledDensity: number = display.getDefaultDisplaySync().scaledDensity;

export default class MyAbilityStage extends WebAbilityStage {
  private static isFirstLaunch: boolean = true;
  tag = "QtForOpenHarmony";
  domain = 0xFF00;

  // 应用启动时调用：初始化 CEF 适配层（传入屏幕密度），并将当前 AbilityStage 注册到 QPA 插件
  onCreate(): void {
    adapter_c.initFunc(scaledDensity);
    qpa.attachAbilityStage(this);
    super.onCreate();
  }

  /**
   * 多窗口模式下决定哪个 Ability 实例响应这次 Want 请求。
   * - 首次启动返回 "qt_entry_window"；
   * - 后续启动由 QPA 插件返回已有窗口的 instanceKey；
   * - 若 Want 中携带 instanceKey 则直接使用该 Key。
   */
  onAcceptWant(want: Want): string {
    hilog.info(this.domain, this.tag, 'onAcceptWant want：%{public}s',
      JSON.stringify(want.parameters));
    if (want.parameters?.instanceKey == undefined) {
      if (MyAbilityStage.isFirstLaunch) {
        MyAbilityStage.isFirstLaunch = false;
        return "qt_entry_window";
      } else {
        let instanceKey: string | undefined = qpa.getJsAbilityInstanceKey();
        if (instanceKey != undefined) {
          return instanceKey
        } else {
          return "qt_entry_window";
        }
      }
    }
    return `${want.parameters?.instanceKey}`;
  }
}
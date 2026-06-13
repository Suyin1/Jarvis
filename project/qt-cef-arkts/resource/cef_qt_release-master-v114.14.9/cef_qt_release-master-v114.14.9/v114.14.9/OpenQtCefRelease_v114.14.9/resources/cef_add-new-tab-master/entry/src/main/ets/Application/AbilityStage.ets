// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import adapter_c from 'libadapter_c.so';
import { WebAbilityStage } from 'web_engine';
import display from '@ohos.display';
import { AbilityStage } from '@kit.AbilityKit';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';

let scaledDensity: number = display.getDefaultDisplaySync().scaledDensity;

export default class MyAbilityStage extends WebAbilityStage {
  private static isFirstLaunch: boolean = true;
  tag = "QtForOpenHarmony";
  domain = 0xFF00;

  onCreate(): void {
    adapter_c.initFunc(scaledDensity);
    qpa.attachAbilityStage(this);
    super.onCreate();
  }

  /**
   * 返回ability的Key，如果已存在则直接拉起，否则创建新的ability。
   * @param want
   * @returns
   */
  onAcceptWant(want: Want): string {
    hilog.info(this.domain, this.tag, 'onAcceptWant want：%{public}s',
      JSON.stringify(want.parameters));
    //创建新主窗口时instanceKey不为undefined
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
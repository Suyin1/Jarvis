// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';
import { Configuration } from '@ohos.app.ability.Configuration';
import { WebAbility } from 'web_engine';
import { WindowManager } from '@kit.SpeechKit';

export default class EntryAbility extends WebAbility {
  tag = 'QtForOpenHarmony';
  domain = 0xFF00;
  private launchApplication = 'libqtmodule.so';
  private launchParams = '';
  private name: string = 'qt_entry_window';
  private loadContentUrl: string = 'pages/Index';

  onConfigurationUpdate(config: Configuration) {
    super.onConfigurationUpdate(config);
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    super.onCreate(want, launchParam);
    let name = want.parameters?.windowName as string;
    if (name != undefined && name !== '') {
      this.name = name;
    }

    let applicationContext = this.context.getApplicationContext();
    applicationContext.on('applicationStateChange', {
      onApplicationForeground() {
        console.log('applicationStateChangeCallback onApplicationForeground');
      },
      onApplicationBackground() {
        console.log('applicationStateChangeCallback onApplicationBackground');
      }
    });
  }

  newLocalStorage() {
    return new LocalStorage();
  }

  onDestroy() {
    super.onDestroy();
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  onWindowStageCreate(windowStage: Window.WindowStage) {
    // Main window is created, set main page for this ability
    WindowManager.setWindowStage(windowStage);
    super.onWindowStageCreate(windowStage);
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);
    let newAbility = this.launchWant.parameters?.newAbility as boolean;

    windowStage.loadContent(this.loadContentUrl, localStore).then(() => {
      qpa.handleJsTopWindowCreated(this.name, this);
      if (newAbility !== undefined) {
        this.context.setMissionLabel(this.launchWant.parameters?.title as string);
        return;
      }
    });

    if (newAbility !== undefined) {
      return;
    }
    this.launchParamSet();
    qpa.startQtApplication(this);
  }

  async launchParamSet() {
    let params: string[] = [];
    if (this.launchWant.uri != undefined && this.launchWant.uri != '') {
      params.push(this.launchWant.uri);
    }

    //NOTE：应用启动参数在此处使用params.push添加
    this.launchParams = params.join(' ');
  }

  onWindowStageDestroy() {
    // Main window is destroyed, release UI related resources
    super.onWindowStageDestroy();
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    // Ability has brought to foreground
    super.onForeground();
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    // Ability has back to background
    super.onBackground();
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  /**
   * note：启动模式为singleton/specified才生效
   * @param want
   * @param launchParam
   */
  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    qpa.handleOnNewWant(want, launchParam);
  }
};
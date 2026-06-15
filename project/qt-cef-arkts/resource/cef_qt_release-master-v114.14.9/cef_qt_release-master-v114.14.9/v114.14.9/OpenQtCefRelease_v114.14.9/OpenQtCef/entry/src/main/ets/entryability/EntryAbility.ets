// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ============================================================
// EntryAbility —— 主窗口 Ability（Qt + CEF 主入口）
// 功能：管理主窗口生命周期，加载 Index 页面，通过 QPA 插件
//       启动 Qt 应用和 CEF 浏览器。
// ============================================================

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import qpa from 'libplugins_platforms_qopenharmony.so';
import hilog from '@ohos.hilog';
import { Configuration } from '@ohos.app.ability.Configuration';
import { WebQtAbility } from 'web_engine';

export default class EntryQtAbility extends WebQtAbility {
  tag = "QtForOpenHarmony";
  domain = 0xFF00;
  private launchApplication = "libqtmodule.so"  // 需要启动的 Qt SO 库
  private launchParams = ""                       // 传递给 Qt 应用的启动参数
  private name: string = 'qt_entry_window'       // 窗口标识名
  private loadContentUrl: string = 'pages/Index'  // 主页面路由

  onConfigurationUpdate(config: Configuration) {
    super.onConfigurationUpdate(config);
  }

  // Ability 创建时：从 Want 中提取窗口名称，注册前后台状态监听
  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    super.onCreate(want, launchParam);
    let name = want.parameters?.windowName as string;
    if (name != undefined && name !== '')
      this.name = name

    let applicationContext = this.context.getApplicationContext();
    applicationContext.on('applicationStateChange', {
      onApplicationForeground() {
        console.log('applicationStateChangeCallback onApplicationForeground');
      },
      onApplicationBackground() {
        console.log('applicationStateChangeCallback onApplicationBackground');
      }
    })
  }

  newLocalStorage() {
    return new LocalStorage()
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  // 窗口创建完成：加载 Index 页面，通知 QPA 窗口已创建，然后启动 Qt 应用
  onWindowStageCreate(windowStage: Window.WindowStage) {
    super.onWindowStageCreate(windowStage);
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);
    let newAbility = this.launchWant.parameters?.newAbility as boolean;

    windowStage.loadContent(this.loadContentUrl, localStore).then(()=>{
      qpa.handleJsTopWindowCreated(this.name, this);
      if (newAbility !== undefined) {
        this.context.setMissionLabel(this.launchWant.parameters?.title as string);
        return;
      }
    })

    if (newAbility !== undefined)
      return;
    this.launchParamSet();
    qpa.startQtApplication(this);
  }

  // 组装启动参数（从 Want URI 中提取），最终传递给 QPA 启动的 Qt 应用
  async launchParamSet(){
    let params:string[] = [];
    if(this.launchWant.uri != undefined && this.launchWant.uri != ""){
      params.push(this.launchWant.uri)
    }

    this.launchParams = params.join(' ');
  }

  onWindowStageDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageDestroy');
  }

  onForeground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  // 新的 Want 到达时（singleton/specified 模式），转发给 QPA 处理
  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    qpa.handleOnNewWant(want, launchParam);
  }
};
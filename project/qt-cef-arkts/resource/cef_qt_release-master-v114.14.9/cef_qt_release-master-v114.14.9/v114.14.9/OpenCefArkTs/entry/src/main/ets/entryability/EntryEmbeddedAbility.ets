// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ============================================================
// EntryEmbeddedAbility —— 嵌入式 Ability（内嵌子窗口）
// 功能：通过 EmbeddedUIExtensionAbility 将 Qt/CEF 渲染内容
//       嵌入到其他应用或父窗口中，用于子窗口/标签页嵌入场景。
// ============================================================

import { hilog } from '@kit.PerformanceAnalysisKit';
import { UIExtensionContentSession, Want } from '@kit.AbilityKit';
import EmbeddedUIExtensionAbility from '@ohos.app.ability.EmbeddedUIExtensionAbility';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import qpa from 'libplugins_platforms_qopenharmony.so';

export default class EntryEmbeddedAbility extends EmbeddedUIExtensionAbility {
  tag = "QtForOpenHarmony";
  domain = 0x0000;
  private name: string = "qt_entry_window"
  private launchParams = ""
  private launchApplication = "libentry.so"
  private launchWant: Want  = {}
  private loadContentUrl: string = 'pages/Index'

  onCreate(launchParam: AbilityConstant.LaunchParam) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onCreate');
  }

  onForeground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onForeground');
  }

  onBackground() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onBackground');
  }

  onDestroy() {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onDestroy');
  }

  // 嵌入式 Session 创建时：将 Index 页面加载到 UIExtensionContentSession 中，
  // 并将 UIExtensionContentSession 对象存入 LocalStorage，供 Qt 表面渲染使用
  onSessionCreate(want: Want, session: UIExtensionContentSession) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onSessionCreate');
    this.launchWant = want;
    let localStore = new LocalStorage();
    localStore.setOrCreate('session', session);
    localStore.setOrCreate('idName', this.name);
    session.loadContent(this.loadContentUrl, localStore)
    let newAbility = this.launchWant.parameters?.newAbility as boolean;
    if (newAbility !== undefined)
      return;
    this.launchParamSet();
    qpa.startQtApplication(this, session);
  }

  newLocalStorage() {
    return new LocalStorage();
  }

  async launchParamSet(){
    let params:string[] = [];
    if(this.launchWant.uri != undefined){
      params.push(this.launchWant.uri)
    }

    this.launchParams = params.join(' ');
  }

  onSessionDestroy(session: UIExtensionContentSession) {
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onSessionDestroy');
  }
};


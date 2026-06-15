// ============================================================
// EntryAbility —— 主窗口 Ability（纯 CEF + ArkTS 主入口）
// 功能：管理主窗口生命周期，加载 Index 页面。
//       与 Qt 版本不同，此处不继承 WebQtAbility，不调用 QPA 插件。
//       CEF 初始化由 adapter_c + CefBridge 在 Ability 生命周期中完成。
// ============================================================

import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import Window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import hilog from '@ohos.hilog';
import { Configuration } from '@ohos.app.ability.Configuration';
import { WebAbility } from 'web_engine';

export default class EntryAbility extends WebAbility {
  tag = "CefForOpenHarmony";
  domain = 0xFF00;
  private name: string = 'cef_entry_window'
  private loadContentUrl: string = 'pages/Index'

  onConfigurationUpdate(config: Configuration) {
    super.onConfigurationUpdate(config);
  }

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

  onWindowStageCreate(windowStage: Window.WindowStage) {
    super.onWindowStageCreate(windowStage);
    hilog.info(this.domain, this.tag, '%{public}s', 'Ability onWindowStageCreate');
    let localStore = new LocalStorage();
    localStore.setOrCreate('idName', this.name);

    windowStage.loadContent(this.loadContentUrl, localStore).then(() => {
      // 页面加载完成后，通知 C++ 层启动 CEF
      // CefBridge 已在 AbilityStage.onCreate 中注册桥接回调
      console.info('EntryAbility: Index page loaded, CEF surface ready');
    })
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

  onNewWant(want: Want, launchParam: AbilityConstant.LaunchParam): void {
    // 纯 CEF 版本：无需 QPA 转发，可在此处处理新 Want
  }
};

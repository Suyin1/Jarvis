// ============================================================
// AbilityStage —— 应用级生命周期入口（纯 CEF + ArkTS 版本）
// 功能：初始化 CEF 适配层 (adapter_c)、注册桥接回调、
//       管理多窗口 Ability 实例键分发。
//       与 Qt 版本不同，此处不关联 QPA 插件。
// ============================================================

import adapter_c from 'libadapter_c.so';
import { WebAbilityStage } from 'web_engine';
import display from '@ohos.display';
import { AbilityStage } from '@kit.AbilityKit';
import Want from '@ohos.app.ability.Want';
import hilog from '@ohos.hilog';

let scaledDensity: number = display.getDefaultDisplaySync().scaledDensity;

export default class MyAbilityStage extends WebAbilityStage {
  private static isFirstLaunch: boolean = true;
  tag = "CefForOpenHarmony";
  domain = 0xFF00;

  onCreate(): void {
    // 初始化 CEF 适配层（传入屏幕密度）
    adapter_c.initFunc(scaledDensity);

    // 初始化 CEF 引擎：注册桥接回调 + 启动 CEF 消息循环线程
    // 对应 C++ 层：
    //   CefBridge::GetInstance()->RegisterBridges()
    //   CefBridge::GetInstance()->StartCef()
    adapter_c.cefInit();

    super.onCreate();
  }

  onAcceptWant(want: Want): string {
    hilog.info(this.domain, this.tag, 'onAcceptWant want：%{public}s',
      JSON.stringify(want.parameters));
    if (want.parameters?.instanceKey == undefined) {
      if (MyAbilityStage.isFirstLaunch) {
        MyAbilityStage.isFirstLaunch = false;
        return "cef_entry_window";
      }
      return "cef_entry_window";
    }
    return `${want.parameters?.instanceKey}`;
  }
}

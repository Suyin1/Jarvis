// ============================================================
// Index1 —— 测试页面（剪贴板功能验证）
// 功能：用于测试 HarmonyOS 系统剪贴板的读取能力，
//       点击按钮获取系统剪贴板文本。此页面为开发调试用途。
// ============================================================

import uri from '@ohos.uri';
import { JsPasteBoard } from '../native/QtCore/JsPasteBoard'
import pasteboard from '@ohos.pasteboard';
@Entry
@Component
struct Index1 {
  build() {
    Column() {
      Button("test").width("100%").onClick((event)=>{
        let systemPasteboard = pasteboard.getSystemPasteboard();
        let pasteData = systemPasteboard.getData();
        pasteData.then(p=>{
          console.log("剪贴板内容:", p.getPrimaryText())
        });
      })
    }
  }
}

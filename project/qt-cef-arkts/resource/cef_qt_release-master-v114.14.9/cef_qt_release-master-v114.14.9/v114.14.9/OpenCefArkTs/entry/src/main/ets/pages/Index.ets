// ============================================================
// Index —— 主窗口页面（纯 CEF + ArkTS 版本）
// 功能：创建 XComponent (SURFACE) 作为 CEF 渲染表面的宿主容器。
//       与 Qt 版本不同，此处不使用 QPA 插件 (NODE 模式)，
//       而是直接使用 SURFACE 模式，由 adapter_c 管理原生节点。
//       CEF 直接渲染到 XComponent Surface。
// ============================================================

import CustomChildProcess from '../process/CustomChildProcess';
import adapter_c from 'libadapter_c.so';

CustomChildProcess.toString();
let storage = LocalStorage.getShared();
@Entry(storage)
@Component
struct Index {
  @LocalStorageProp("idName") windowId: string = "";

  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      XComponent({
        id: this.windowId,
        type: XComponentType.SURFACE,
        // 纯 CEF 版本：XComponent 类型为 SURFACE，不加载 QPA 插件
        // CEF 通过 adapter_c 的 FrameNode API 直接渲染到 Surface
      })
        .width('100%')
        .height('100%')
        .backgroundColor(Color.White)
        .onLoad((context) => {
          // Surface 加载完成后，通知 C++ 层 Surface 窗口句柄
          // context 是 SURFACE 模式的原生 surface 上下文，
          // C++ 侧通过 OH_NativeXComponent API 从中提取 OHNativeWindow，
          // 然后 CefBridge::OnSurfaceReady 将创建 CEF 浏览器实例
          console.info('Index: XComponent Surface loaded, notifying CEF bridge');
          adapter_c.cefSurfaceReady(this.windowId, context);
        })
    }
    .width('100%')
    .height('100%')
  }
}

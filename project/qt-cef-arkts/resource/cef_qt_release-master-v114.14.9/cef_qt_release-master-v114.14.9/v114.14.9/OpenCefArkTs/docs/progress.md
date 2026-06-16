# 编译进度 — OpenCefArkTs

## 环境
- DevEco Studio: D:\software\DevEco Studio
- SDK: API 26 Beta1 (platform 26.0.0)
- 目标: arm64-v8a, stage 模式
- 构建命令: `node hvigorw.js --mode module -p product=default assembleHap`

## 修复汇总

### ArkTS 编译错误 (已解决)
| 文件 | 行 | 修复 |
|---|---|---|
| `EntryAbility.ets` | 46 | `onDestroy()` → `async onDestroy(): Promise<void>` + `await super.onDestroy()` |
| `OneKeyLogin.ets` | 45 | `GradientDirection.BottomRight` → `GradientDirection.RightBottom` |
| `OneKeyLogin.ets` | 51 | `Stack.justifyContent(FlexAlign.Center)` → `Stack.alignContent(Alignment.Center)` |
| `OneKeyLogin.ets` | 105 | `Checkbox.checked(bool)` → `Checkbox.select(bool)` |

### C++ 编译错误 (已解决)
| 文件 | 行 | 修复 |
|---|---|---|
| `CefBridge.cpp` | +`#include <hilog/log.h>` | 添加日志头文件 |
| `adapter_c.cpp` | 1159 | 添加 `static constexpr unsigned int LOG_PRINT_DOMAIN` 外部定义 |
| `adapter_c.cpp` | 510 | 前置声明 `DefaultNavigateBridge` |
| `adapter_c.cpp` | 1231 | `EWAdapterC::RegisterArkTSNavigateCallback` → `EMBEDDED_WINDOW_ADAPTER::EWAdapterC::RegisterArkTSNavigateCallback` |
| `adapter_c.cpp` | 1243+ | `DefaultNavigateBridge` 中 `EWAdapterC::` → `EMBEDDED_WINDOW_ADAPTER::EWAdapterC::` |
| `adapter_c.cpp` | 1376-1393 | 移除已废弃的 `OH_NativeXComponent_GetNativeXComponent` / `GetNativeWindow`，替换为 API 26 新版：`OH_ArkUI_GetNodeHandleFromNapiValue` + `OH_ArkUI_SurfaceHolder_Create` + `OH_ArkUI_XComponent_GetNativeWindow` |
| `adapter_c.cpp` | +includes | 添加 `<arkui/native_node_napi.h>` 和 `<native_window/external_window.h>` |
| `TCSimpleHandler.cpp` | 22-26 | 用 `__has_include` 保护已删除的 HMS `ohos/adapter/window/` 头文件 |
| `TCSimpleHandler.cpp` | 33 | 静态成员 `navigate_callback_` 从匿名 `namespace` 移出到全局域 |
| `TCSimpleHandler.cpp` | 111-132 | 用 `__has_include` 保护 `PlatformTitleChange` / `OnFullscreenModeChange` 实现 |

### SDK 版本配置 (已解决)
- 用户手动修改 `build-profile.json5` 适配 API 26 (26.0.0)
- 移除了 `hvigor` modelVersion 冲突

### 构建结果
- **ArkTS**: 编译通过
- **C++ 编译/链接**: 通过
- **签名**: 未通过 — `storePassword`/`keyPassword` 长度 < 32，需在 DevEco Studio 中自动配置签名

## 待办
- [ ] 在 DevEco Studio 中配置签名 (Project Structure > Signing Configs)
- [ ] 构建 HAP 包并部署到设备/模拟器
- [ ] 验证 P0 全链路: 启动 → 加载 home.html → navigateToNative → 返回 HTML
- [ ] 运行时验证新 XComponent API (`OH_ArkUI_GetNodeHandleFromNapiValue`) 能否正确获取 NativeWindow

export const initFunc: (scaledDensity: number) => void;
export const navigateToNative: (pageName: string, callback: (errCode: number, result: string) => void) => void;
export const cefLoadUrl: (url: string) => void;
export const registerNavigateCallback: (callback: (pageName: string) => void) => void;
export const getWindowNameByXComponentId: (id: string) => string;
export const cefInit: () => void;
// cefSurfaceReady — 通知 C++ 层 XComponent Surface 已就绪
// id: XComponent 的 ID（对应 Index.ets 中的 windowId）
// context: XComponent.onLoad 回调传入的 surface context 对象
// C++ 侧通过 OH_NativeXComponent API 从中提取 OHNativeWindow 句柄
export const cefSurfaceReady: (id: string, context: Object) => void;

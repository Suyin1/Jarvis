// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 导航绑定 — 注册 CEF → ArkTS 页面跳转回调
// 当 CEF JS 调用 window.ClientAPI.navigateToNative('OneKeyLogin', callback) 时，
// 经过 V8 → TCSimpleHandler → adapter_c 链路，最终调用此处注册的 ArkTS 函数

import adapter_c from 'libadapter_c.so';
import router from '@ohos.router';

export class NavigationBind {
  static bind() {
    adapter_c.registerNavigateCallback((pageName: string) => {
      console.info(`NavigationBind: navigate to native page: ${pageName}`);
      try {
        router.pushUrl({
          url: `pages/${pageName}`,
          params: { source: 'cef_html' }
        });
      } catch (err) {
        console.error(`NavigationBind: pushUrl failed for ${pageName}: ${err}`);
      }
    });
  }
}

// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ============================================================
// SubWindow —— 子窗口页面
// 功能：通过 WebSubWindow 组件渲染 CEF 子窗口内容，
//       用于浏览器内部子标签页或嵌入式子窗口场景。
// ============================================================

import { WebSubWindow } from 'web_engine';

let subStorage = LocalStorage.getShared();
@Entry(subStorage)
@Component
struct SubWindow {
  build() {
    Column() {
      WebSubWindow()
    }
    .width('100%')
    .height('100%')
  }
}
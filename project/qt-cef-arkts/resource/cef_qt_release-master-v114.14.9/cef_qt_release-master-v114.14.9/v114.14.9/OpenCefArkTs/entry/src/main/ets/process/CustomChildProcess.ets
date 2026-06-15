// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ============================================================
// CustomChildProcess —— CEF 子进程入口
// 功能：扩展 WebChildProcess 基类，用于 CEF 渲染进程/
//       子进程启动时的初始化逻辑（当前仅代理父类实现）。
// ============================================================

import { WebChildProcess } from 'web_engine/childProcess';

export default class CustomChildProcess extends WebChildProcess {
  onStart(): void {
    super.onStart();
  }

  static toString(): string {
    return "CustomChildProcess";
  }
}
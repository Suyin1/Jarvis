// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 公共模块 — 注册全局单例服务到 IoC 容器
// 包含 AbilityManager、DragParamManager、SystemFloatingWindowManager

import { ContainerModule } from 'inversify';

import { AbilityManager } from '../common/AbilityManager';
import { DragParamManager } from '../common/DragParamManager';
import { SystemFloatingWindowManager } from './SystemFloatingWindowManager';

export const commonModule = new ContainerModule((bind) => {
  bind(AbilityManager).toSelf();
  bind(DragParamManager).toSelf();
  bind(SystemFloatingWindowManager).toSelf();
});

// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 适配器模块 — 注册 WebAbility/WebWindow 所需的 HarmonyOS 平台适配器
// 通过 InversifyJS ContainerModule 完成 DI 绑定

import { ContainerModule } from 'inversify';


import lazy { ContextAdapter } from '../adapter/ContextAdapter';
import lazy { ContextPathAdapter } from '../adapter/ContextPathAdapter';
import lazy { DisplayAdapter } from '../adapter/DisplayAdapter';
import lazy { DragDropAdapter } from '../adapter/DragDropAdapter';
import lazy { MultiInputAdapter } from '../adapter/MultiInputAdapter';
import lazy { NativeThemeAdapter } from '../adapter/NativeThemeAdapter';
import lazy { PermissionManagerAdapter } from '../adapter/PermissionManagerAdapter';
import lazy { DeviceInfoAdapter } from '../adapter/DeviceInfoAdapter';

// 仅注册 WebAbility 或 WebWindow 中使用的适配器
export const adapterModule = new ContainerModule((bind) => {
  bind(ContextAdapter).toSelf();
  bind(DisplayAdapter).toSelf();
  bind(DragDropAdapter).toSelf();
  bind(MultiInputAdapter).toSelf();
  bind(NativeThemeAdapter).toSelf();
  bind(ContextPathAdapter).toSelf();
  bind(PermissionManagerAdapter).toSelf();
  bind(DeviceInfoAdapter).toSelf();
});

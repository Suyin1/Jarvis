// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// DI 依赖接口定义 — Dependency 提供 Context、CommonModule、AdapterModule
// DependencyProvider 是工厂接口，用于创建 Dependency 实例

import { ContainerModule } from 'inversify';
import { common } from '@kit.AbilityKit';

export interface Dependency {
  getContext: () => common.Context;
  getCommonModule: () => ContainerModule;
  getAdapterModule: () => ContainerModule;
}

export interface DependencyProvider {
  createDependency: () => Dependency;
}

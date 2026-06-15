// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 公共依赖提供器 — 创建应用级 DI 依赖注入实例
// CommonDependency 封装了 Context、commonModule 和 adapterModule
// 供 InjectModule 初始化 IoC 容器使用

import { common } from '@kit.AbilityKit';
import { ContainerModule } from 'inversify';
import { Dependency, DependencyProvider } from '../interface/Dependency';
import { commonModule,  } from './CommonModule';
import { adapterModule } from './AdapterModule';

class CommonDependency implements Dependency {
  private context: common.Context;

  constructor(ctx: common.Context) {
    this.context = ctx;
  }

  public getContext(): common.Context {
    return this.context;
  }

  getCommonModule(): ContainerModule {
    return commonModule;
  }

  getAdapterModule(): ContainerModule {
    return adapterModule;
  }
}

export class CommonDependencyProvider implements DependencyProvider {
  private context: common.Context;

  constructor(ctx: common.Context) {
    this.context = ctx;
  }

  public createDependency(): Dependency {
    return new CommonDependency(this.context);
  }
}
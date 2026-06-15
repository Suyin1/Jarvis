// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 拖拽适配器 JsBinding — 注册拖拽事件处理函数到 C++，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { DragDropAdapter } from '../adapter/DragDropAdapter';
import type { OhosDragParamToJs } from '../interface/CommonInterface';

function implDragDropAdapter(): DragDropAdapter {
  return Inject.getOrCreate(DragDropAdapter);
}

function startDrag(dragParams: OhosDragParamToJs) {
  implDragDropAdapter().startDrag(dragParams);
}

export class DragDropAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction('DragDropAdapter.StartDrag', startDrag);
  }
}

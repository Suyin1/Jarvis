// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 无障碍服务适配器 — 封装 @ohos.accessibility API，为 CEF 提供无障碍访问支持
// 继承 BaseAdapter，实现无障碍服务的语音播报功能

import { injectable } from 'inversify';
import { BaseAdapter } from '../common/BaseAdapter';
import LogMethod from '../common/LogDecorator';
import LogUtil from '../utils/LogUtil';

const TAG: string = 'Accessibility';

@injectable()
export class AccessibilityAdapter extends BaseAdapter {
  @LogMethod
  speech(text: string, id: string) {
    LogUtil.info(TAG, `speech NOTIMPLEMENTED`);
  }

  @LogMethod
  shutDown() {
    LogUtil.info(TAG, `shutDown NOTIMPLEMENTED`);
  }
}
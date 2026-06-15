// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// 国际化适配器 JsBinding — 注册本地化函数到 C++，供 CEF 层调用

import JsBindingUtils from '../utils/JsBindingUtils';
import Inject from '../common/InjectModule';
import lazy { I18nAdapter } from '../adapter/I18nAdapter';

function implI18nAdapter(): I18nAdapter {
  return Inject.getOrCreate(I18nAdapter);
}

function getLocaleLang(): string {
  return implI18nAdapter().getLocaleLang();
}

function getLocaleRegion(): string {
  return implI18nAdapter().getLocaleRegion();
}

function registerTimeZoneListener(callback: Function) {
  implI18nAdapter().registerTimeZoneListener(callback);
}

function unsubscribeTimeZoneListener() {
  implI18nAdapter().unsubscribeTimeZoneListener();
}

export class I18nAdapterBind {
  static bind() {
    JsBindingUtils.bindFunction("OhosI18nAdapter.GetLocaleLang", getLocaleLang);
    JsBindingUtils.bindFunction("OhosI18nAdapter.getLocaleRegion", getLocaleRegion);
    JsBindingUtils.bindFunction("OhosI18nAdapter.RegisterTimeZoneListener", registerTimeZoneListener);
    JsBindingUtils.bindFunction("OhosI18nAdapter.UnsubscribeTimeZoneListener", unsubscribeTimeZoneListener);
  }
}

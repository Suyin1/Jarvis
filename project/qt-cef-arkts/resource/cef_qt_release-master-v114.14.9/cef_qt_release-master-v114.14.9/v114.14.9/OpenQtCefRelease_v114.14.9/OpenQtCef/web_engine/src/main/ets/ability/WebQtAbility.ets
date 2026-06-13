// Copyright (c) 2026 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import UIAbility from '@ohos.app.ability.UIAbility';
import LogUtil from '../utils/LogUtil';
import window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import JsBindingUtils from '../utils/JsBindingUtils';
import { NativeContext, WindowBound } from '../interface/CommonInterface';
import { NativeThemeAdapter } from '../adapter/NativeThemeAdapter';
import { AbilityManager } from '../common/AbilityManager';
import { Configuration } from '@ohos.app.ability.Configuration';
import Inject from '../common/InjectModule';
import { DisplayAdapter } from  '../adapter/DisplayAdapter';
import { PermissionManagerAdapter } from '../adapter/PermissionManagerAdapter';
import { WebProxy } from '../interface/WebProxy';
import { common } from '@kit.AbilityKit';
import { BusinessError } from '@kit.BasicServicesKit';
import { ContextType } from '../common/Constants';
import { ContextAdapter } from '../adapter/ContextAdapter';

const TAG: string = 'WebQtAbility';

export class WebQtAbility extends UIAbility implements WebProxy {
  protected xcomponentId: string = '';
  protected windowStage: window.WindowStage | undefined = undefined;
  protected nativeContext: NativeContext =
    JsBindingUtils.getNativeContext(ContextType.kMainProcess);
  protected originWindowId: number = -1;
  private abilityManager: AbilityManager | undefined = undefined;
  private displayAdapter : DisplayAdapter | undefined = undefined;
  private nativeThemeAdapter: NativeThemeAdapter | undefined = undefined;
  private contextAdapter: ContextAdapter | undefined = undefined;

  onConfigurationUpdate(config: Configuration) {
    this.nativeThemeAdapter?.setSystemNativeTheme(config.colorMode);
    this.displayAdapter?.onSystemFontSizeChange(config.fontSizeScale);
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    this.abilityManager = Inject.get(AbilityManager);
    this.nativeThemeAdapter = Inject.get(NativeThemeAdapter);
    this.displayAdapter = Inject.get(DisplayAdapter);
    this.contextAdapter = Inject.get(ContextAdapter);
    Inject.get(PermissionManagerAdapter).initPermissions();
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    this.windowStage = windowStage;
    this.originWindowId = windowStage?.getMainWindowSync().getWindowProperties().id;
    this.abilityManager?.addProxy(this);
    if (this.abilityManager?.getProxyCount() === 1) {
      let prop = windowStage?.getMainWindowSync().getWindowProperties();
      if (prop) {
        LogUtil.info(TAG, 'lxh------onWindowStageCreate--prop:' + JSON.stringify(prop));
        let windowRect: WindowBound = {
          left: prop.windowRect.left,
          top: prop.windowRect.top,
          width: prop.windowRect.width,
          height: prop.windowRect.height
        }

        let drawableRect: WindowBound = {
          left: prop.drawableRect.left,
          top: prop.drawableRect.top,
          width: prop.drawableRect.width,
          height: prop.drawableRect.height
        }

        this.nativeContext.OnWindowInitSize(windowRect, drawableRect);
      }
    }
  }

  // implements WebProxy interface
  getWindowStage(): window.WindowStage | undefined {
    return this.windowStage;
  }

  getWindow(): window.Window | undefined {
    return this.windowStage?.getMainWindowSync();
  }

  getWindowContext(): common.Context {
    return this.context;
  }

  getWindowId(): string {
    return this.xcomponentId;
  }

  closeWindow() {
    this.context.terminateSelf()
      .then(() => {
        LogUtil.info(TAG, 'terminate Self id: ' + this.getWindowId());
      })
      .catch((err: BusinessError) => {
        LogUtil.error(TAG, 'Failed to terminate Self: ' + JSON.stringify(err));
      });
  }

  setWindowTitle(title:string) {
    this.context.setMissionLabel(title)
      .catch((err: BusinessError) => {
        LogUtil.error(TAG, 'Failed to set title: ' + JSON.stringify(err));
      });
  }

  createSubWindow(name: string): Promise<window.Window> {
    return new Promise<window.Window>((resolve, reject) => {
      if (!this.windowStage) {
        reject('Failed to get windowStage');
        return;
      }
      this.windowStage.createSubWindow(name)
        .then((data: window.Window) => {
          resolve(data);
        }).catch((error: BusinessError) => {
        reject(error);
      })
    });
  }

  getOriginWindowId(): number {
    return this.originWindowId;
  }

  getIsQtChild(): boolean {
    return true;
  }

  isUIAbility(): boolean {
    return true;
  }

  updateAvoidAreaStatus(isFullScreen: boolean) {}
};

// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import window from '@ohos.window';
import Want from '@ohos.app.ability.Want';
import AbilityConstant from '@ohos.app.ability.AbilityConstant';
import { Configuration } from '@ohos.app.ability.Configuration';
import { WebAbility } from './WebAbility';
import { deviceInfo, settings } from '@kit.BasicServicesKit';
import { common, ConfigurationConstant } from '@kit.AbilityKit';
import LogUtil from '../utils/LogUtil';
import CheckEmptyUtils from '../utils/CheckEmptyUtils';
import { AbilityManager } from '../common/AbilityManager';
import { CommandType, ConfigData } from '../common/Constants';
import { BrowserCloseResponse, WindowBound } from '../interface/CommonInterface';
import { DeviceInfoAdapter } from '../adapter/DeviceInfoAdapter';
import Inject from '../common/InjectModule';


const TAG: string = 'WebEntryAbilityPopup';

export default class WebEntryAbilityPopup extends WebAbility {

  protected static WINDOW_CLOSE: number = 1000;
  protected static SLEEP_INTERVAL_MS = 10;
  protected static MAXIMUM_NUM_OF_SLEEP_CYCLES = 250;
  private deviceInfoAdapter: DeviceInfoAdapter | undefined = undefined;

  onConfigurationUpdate(config: Configuration) {
    super.onConfigurationUpdate(config);
  }

  onCreate(want: Want, launchParam: AbilityConstant.LaunchParam) {
    super.onCreate(want, launchParam);
    this.deviceInfoAdapter = Inject.get(DeviceInfoAdapter);
  }

  onWindowStageCreate(windowStage: window.WindowStage) {
    if (this.checkSingleInstance()) {
      LogUtil.warn(TAG, 'repeat launch from desktop, need terminate');
      return;
    }
    if (deviceInfo.sdkApiVersion >= 14) {
      windowStage.setWindowRectAutoSave(true);
    }
    windowStage.getMainWindow((err, data) => {
      if (err.code) {
        LogUtil.error(TAG, 'Failed to obtain the main window: ' + JSON.stringify(err));
        return;
      }
      try {
        data.on('windowStatusChange', (status) => {
          this.nativeContext.OnWindowStatusChange(this.xcomponentId, status);
        })
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window status changes: ' + JSON.stringify(exception));
      }

      try {
        data.on('windowSizeChange', (size) => {
          const prop = data.getWindowProperties();
          let windowBound: WindowBound = {
            left: prop.windowRect.left + prop.drawableRect.left,
            top: this.hideTitleBar ? prop.windowRect.top + prop.drawableRect.top : prop.windowRect.top,
            width: size.width - prop.drawableRect.left - prop.drawableRect.left,
            height: this.hideTitleBar ? size.height - prop.drawableRect.left - prop.drawableRect.top : size.height - prop.drawableRect.top,
          };
          this.nativeContext.OnWindowSizeChange(this.xcomponentId, windowBound);
        });
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window size changes: ' + JSON.stringify(exception));
      }

      try {
        data.on("windowRectChange", (option) => {
          const prop = data.getWindowProperties();
          const rect = option.rect;
          let statusBarHeight = 0;
          if (this.deviceInfoAdapter?.isNormalWindowMode()) {
            statusBarHeight = data.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM).topRect.height;
            //The current system has a timing issue where the drawableRect lags in updating during mode switching.
            //Here, when switching to normal mode, the drawableRect is set to 0
            //to temporarily maintain normal performance until the issue is resolved.
            prop.drawableRect.height = prop.drawableRect.height + prop.drawableRect.top;
            prop.drawableRect.top = 0;
          }
          let windowBound: window.Rect = {
            left: rect.left + prop.drawableRect.left,
            top: rect.top + prop.drawableRect.top + statusBarHeight,
            width: rect.width - prop.drawableRect.left - prop.drawableRect.left,
            height: rect.height - prop.drawableRect.left - prop.drawableRect.top - statusBarHeight,
          };
          this.nativeContext.OnWindowRectChange(this.xcomponentId,
            windowBound,
            option.reason.valueOf());
        });
      } catch (exception) {
        LogUtil.error(TAG,'Failed to listener for window rect changes: ' + JSON.stringify(exception));
      }

      try {
        data.on('windowEvent', (type) => {
          this.nativeContext.OnWindowEvent(this.xcomponentId, type.valueOf());
        });
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to register callback: ' + JSON.stringify(exception));
      }

      try {
        data.on("windowVisibilityChange", (visible) => {
          this.nativeContext.OnWindowVisibilityChange(this.xcomponentId, visible);
        });
      } catch (exception) {
        LogUtil.error(TAG, 'Failed to register callback: ' + JSON.stringify(exception));
      }
    });

    super.onWindowStageCreate(windowStage);

    const para: Record<string, Object> = {
      'xcomponentId': this.xcomponentId,
      'startUri': this.startUri,
      'window': windowStage.getMainWindowSync(),
      'useDarkMode': this.needDarkMode(),
    };
    let storage: LocalStorage = new LocalStorage(para);
    windowStage.loadContent('pages/IndexPopup', storage, (err, data) => {
      if (err.code) {
        LogUtil.error(TAG, 'Failed to load the content: ' + JSON.stringify(err));
        return;
      }

      let window: window.Window = windowStage.getMainWindowSync();
      // hide title bar
      // window.setWindowDecorVisible(!this.hideTitleBar);

      if (deviceInfo.sdkApiVersion >= 14) {
        if (!this.controlButtonsVisible) {
          window.setWindowTitleButtonVisible(false, false, false);
        } else if (this.useDarkMode && deviceInfo.deviceType === '2in1') {
          let buttonStyle: window.DecorButtonStyle = {
            colorMode: ConfigurationConstant.ColorMode.COLOR_MODE_DARK
          };
          window.setDecorButtonStyle(buttonStyle);
        }
      }

      let isPCStatus = settings.getValueSync(this.context, 'isSystemModePc', '', settings.domainName.USER_PROPERTY);
      if (deviceInfo.deviceType === 'tablet' && isPCStatus !== 'true') {
        this.onPadWindowStageCreate(window, storage);
      }
    });
  }

  async onPrepareToTerminateAsync(): Promise<boolean> {
    let id: number = Number(this.xcomponentId.substring(ConfigData.WINDOW_PREFIX.length));
    let response: BrowserCloseResponse = BrowserCloseResponse.kUndetermined;
    this.nativeContext.OnWindowEvent(this.xcomponentId, WebEntryAbilityPopup.WINDOW_CLOSE);
    for (let i = 0; i < WebEntryAbilityPopup.MAXIMUM_NUM_OF_SLEEP_CYCLES; i++) {
      await this.sleep(WebEntryAbilityPopup.SLEEP_INTERVAL_MS);
      response = this.nativeContext.GetBrowserCloseResponse(id);
      if (response !== BrowserCloseResponse.kUndetermined) {
        break;
      }
    }
    if (response === BrowserCloseResponse.kClosingContinue || response === BrowserCloseResponse.kClosed) {
      return false;
    }
    return true;
  }

  sleep(delay: number) {
    return new Promise<void>((resolve) => setTimeout(resolve, delay));
  }

  async onDestroy(): Promise<void> {
    let id: number = Number(this.xcomponentId.substring(ConfigData.WINDOW_PREFIX.length));
    while (true) {
      await this.sleep(WebEntryAbilityPopup.SLEEP_INTERVAL_MS);
      let response = this.nativeContext.GetBrowserCloseResponse(id);
      if (response === BrowserCloseResponse.kClosed || response === BrowserCloseResponse.kClosedAnyway) {
        break;
      }
    }
    if (this.abilityManager?.getProxyCount() === 0) {
      while (true) {
        await this.sleep(WebEntryAbilityPopup.SLEEP_INTERVAL_MS);
        let destroyed = this.nativeContext.BrowserDestroyed();
        if (destroyed) {
          break;
        }
      }
    }
    return;
  }

  onWindowStageDestroy() {
    super.onWindowStageDestroy();
  }

  onForeground() {
    super.onForeground();
  }

  onBackground() {
    super.onBackground();
  }

  private needDarkMode(): boolean {
    let context = this.getWindowContext() as common.UIAbilityContext;
    let currentColorMode: ConfigurationConstant.ColorMode | undefined = context.config.colorMode;
    if (!currentColorMode) {
      LogUtil.warn(TAG, 'Can not get current color mode from window: ' + this.xcomponentId);
    }
    return this.useDarkMode || currentColorMode === ConfigurationConstant.ColorMode.COLOR_MODE_DARK;
  }
  
  private onPadWindowStageCreate(windowClass: window.Window, storage: LocalStorage) {
    // During window initialization, it is not in full-screen state, so the input is false.
    // Here set the status bar avoidance and hides the navigation bar.
    this.updateAvoidAreaStatus(false);

    let avoidArea = windowClass.getWindowAvoidArea(window.AvoidAreaType.TYPE_SYSTEM);
    let statusBarHeight = avoidArea.topRect.height;
    storage.setOrCreate('statusBarHeight', statusBarHeight);

    windowClass.on('avoidAreaChange', (data) => {
      if (data.type !== window.AvoidAreaType.TYPE_SYSTEM) {
        return;
      }

      let statusBarHeight = data.area.topRect.height;
      storage.setOrCreate('statusBarHeight', statusBarHeight);
      LogUtil.info(TAG, 'Avoidarea changes,statusBarHeight:' + statusBarHeight);

      if(!this.deviceInfoAdapter?.isNormalWindowMode()) {
        return;
      }

      let prop = windowClass.getWindowProperties();
      //The current system has a timing issue where the drawableRect lags in updating during mode switching.
      //Here, when switching to normal mode, the drawableRect is set to 0
      //to temporarily maintain normal performance until the issue is resolved.
      prop.drawableRect.height = prop.drawableRect.height + prop.drawableRect.top;
      prop.drawableRect.top = 0;

      let windowBound: window.Rect = {
        left: prop.windowRect.left + prop.drawableRect.left,
        top: prop.windowRect.top + prop.drawableRect.top + statusBarHeight,
        width: prop.drawableRect.width,
        height: prop.drawableRect.height - statusBarHeight,
      };
      this.nativeContext.OnWindowRectChange(this.xcomponentId, windowBound, 0);
    });
  }
};


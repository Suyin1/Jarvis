// Copyright (c) 2024 Huawei Device Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import CustomChildProcess from '../process/CustomChildProcess';

CustomChildProcess.toString();
let storage = LocalStorage.getShared();
@Entry(storage)
@Component
struct Index {
  @LocalStorageProp("idName") windowId: string = "";
  build() {
    Stack({ alignContent: Alignment.TopStart }) {
      XComponent({
        id: this.windowId,
        type: XComponentType.NODE,
        libraryname: 'plugins_platforms_qopenharmony'
      })
        .width('100%')
        .height('100%')
        .backgroundColor(Color.White)
    }
  }
}
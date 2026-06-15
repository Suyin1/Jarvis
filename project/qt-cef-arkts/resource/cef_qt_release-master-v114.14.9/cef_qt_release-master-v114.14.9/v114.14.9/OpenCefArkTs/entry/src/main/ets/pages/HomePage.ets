// ============================================================
// HomePage —— 原生设置页面（ArkTS）
// 功能：从 HTML 页面通过 navigateToNative 跳转至此页面，
//       验证桥接层完整链路，同时演示 ArkTS 原生 UI 渲染。
// ============================================================

import router from '@ohos.router';

@Entry
@Component
struct HomePage {
  build() {
    Column() {
      // 顶部
      Row() {
        Button({ type: ButtonType.Normal }) {
          Text('<')
            .fontSize(20)
            .fontColor(Color.Black)
        }
        .width(40)
        .height(40)
        .borderRadius(20)
        .backgroundColor('#e7e8ea')
        .onClick(() => {
          router.back()
        })

        Text('设置')
          .fontSize(18)
          .fontWeight(FontWeight.Medium)
          .margin({ left: 16 })
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 8 })

      // 设置列表
      Column() {
        SettingItem({ label: '账号与安全', desc: '修改密码、安全手机等' })
        SettingItem({ label: '通知设置', desc: '管理推送通知' })
        SettingItem({ label: '隐私设置', desc: '控制隐私权限' })
        SettingItem({ label: '关于', desc: '版本信息' })
        SettingItem({ label: '回到 HTML 首页', desc: '跳回 CEF 加载的 H5 页面' })
          .onClick(() => {
            // 通过桥接层方向 B: ArkTS -> CEF
            // 调用 cefLoadUrl 加载本地登录页
          })
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 24 })
      .layoutWeight(1)

      // 底部导航栏
      Row() {
        NavItem({ icon: '\u{1F3E0}', label: '首页', active: true })
        NavItem({ icon: '\u{1F4CB}', label: '订单', active: false })
        NavItem({ icon: '\u{1F464}', label: '我的', active: false })
      }
      .width('100%')
      .height(64)
      .backgroundColor(Color.White)
      .justifyContent(FlexAlign.SpaceAround)
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#f1f3f5')
  }
}

@Component
struct SettingItem {
  private label: string = ''
  private desc: string = ''

  build() {
    Row() {
      Column() {
        Text(this.label)
          .fontSize(16)
          .fontColor(Color.Black)
        Text(this.desc)
          .fontSize(12)
          .fontColor(Color.Gray)
          .margin({ top: 4 })
      }
      .alignItems(HorizontalAlign.Start)

      Blank()
      Text('>')
        .fontSize(16)
        .fontColor(Color.Gray)
    }
    .width('100%')
    .padding(16)
    .backgroundColor(Color.White)
    .borderRadius(12)
    .margin({ bottom: 12 })
  }
}

@Component
struct NavItem {
  private icon: string = ''
  private label: string = ''
  private active: boolean = false

  build() {
    Column() {
      Text(this.icon)
        .fontSize(24)
      Text(this.label)
        .fontSize(10)
        .fontColor(this.active ? '#cd2424' : Color.Gray)
    }
  }
}

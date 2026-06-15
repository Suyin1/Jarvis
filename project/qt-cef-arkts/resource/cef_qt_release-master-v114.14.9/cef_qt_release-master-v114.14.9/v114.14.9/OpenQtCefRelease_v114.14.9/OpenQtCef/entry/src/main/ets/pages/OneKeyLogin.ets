// ============================================================
// OneKeyLogin —— 一键登录（ArkTS 原生页面）
// 功能：展示通过桥接层从 CEF HTML 跳转到 ArkTS 原生页面的效果。
//       页面设计与 accountkit-samplecode QuickLoginPage 风格一致，
//       但不依赖 @kit.AccountKit，仅作 UI 展示和桥接验证。
// ============================================================

import router from '@ohos.router';

@Entry
@Component
struct OneKeyLogin {
  @State isChecked: boolean = true
  @State phoneNumber: string = '180******00'

  build() {
    Column() {
      // 顶部返回区域
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
      }
      .width('100%')
      .padding({ left: 16, right: 16, top: 8 })

      // 中部内容区
      Column() {
        // App 图标
        Stack() {
          Column()
            .width(80)
            .height(80)
            .borderRadius(20)
            .linearGradient({
              direction: GradientDirection.BottomRight,
              colors: [['#667eea', 0], ['#764ba2', 1]]
            })
        }
        .width('100%')
        .height(96)
        .justifyContent(FlexAlign.Center)

        // 手机号
        Text(this.phoneNumber)
          .fontSize(36)
          .fontWeight(FontWeight.Bold)
          .fontColor(Color.Black)
          .lineHeight(48)
          .margin({ top: 24 })

        Text('华为账号绑定号码')
          .fontSize(12)
          .fontColor(Color.Gray)
          .lineHeight(16)
          .margin({ top: 8 })
      }
      .layoutWeight(1)
      .justifyContent(FlexAlign.Center)

      // 底部操作区
      Column() {
        // 一键登录按钮
        Button({ type: ButtonType.Capsule }) {
          Text('一键登录')
            .fontColor(Color.White)
            .fontSize(16)
        }
        .width('100%')
        .constraintSize({ maxWidth: 448 })
        .height(40)
        .backgroundColor('#cd2424')
        .onClick(() => {
          console.info('OneKeyLogin: 一键登录被点击')
          // TODO: 接入 Account Kit 实际登录逻辑
        })

        // 其他登录方式
        Button({ type: ButtonType.Capsule }) {
          Text('其他登录方式')
            .fontColor(Color.Black)
            .fontSize(16)
        }
        .width('100%')
        .constraintSize({ maxWidth: 448 })
        .height(40)
        .backgroundColor('#e7e8ea')
        .margin({ top: 16 })
        .onClick(() => {
          console.info('OneKeyLogin: 切换其他登录方式')
        })

        // 隐私协议
        Row() {
          Checkbox()
            .checked(this.isChecked)
            .width(20)
            .height(20)
            .shape(CheckBoxShape.CIRCLE)
            .onChange((value: boolean) => {
              this.isChecked = value
            })

          Text('已阅读并同意')
            .fontSize(12)
            .fontColor(Color.Gray)
          Text('《用户协议》')
            .fontSize(12)
            .fontColor(Color.Black)
          Text('《隐私政策》')
            .fontSize(12)
            .fontColor(Color.Black)
        }
        .width('100%')
        .constraintSize({ maxWidth: 448 })
        .justifyContent(FlexAlign.Start)
        .margin({ top: 24 })
      }
      .padding({ left: 16, right: 16, bottom: 48 })
    }
    .width('100%')
    .height('100%')
    .backgroundColor('#f1f3f5')
  }
}

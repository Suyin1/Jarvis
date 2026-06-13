一、每个版本可能对应一个或多个zip压缩文件,需要将版本目录下的全部压缩文件进行下载到同一位置,再进行解压缩。
  
  【发布版】
	如:20250328_1, 包含:OpenQtCefRelease_v_20250328_1.zip.001 和 OpenQtCefRelease_v_20250328_1.zip.002
	下载上面两个压缩文件后,点击OpenQtCefRelease_v_20250328_1.zip.001进行解压即可。
  
  【测试版】
	如:v_20250328_1, 包含:OpenQtCefDebug_v_20250328_1.zip.001 和 OpenQtCefDebug_v_20250328_1.zip.002
	下载上面两个压缩文件后,点击OpenQtCefDebug_v_20250328_1.zip.001进行解压即可。

二、工程名:OpenQtCef
  修改项目中 entry\build-profile.json5 下 arguments 字段进行对QT框架的路径适配

三、项目中 entry\libs\arm64-v8a\unstripped 文件夹为debug二进制文件库。

四、本次、历史更新内容:
v114.14.9 更新：
	修复页面中下拉框，放大缩小导致错位
	修复鼠标右键菜单，点击返回，再去点前进时，前进置灰
	修复视频窗口模式点击全屏无法全屏
	修复子进程无法找到自定义so的问题
	关闭过程中网络进程关闭问题修复
	修复播放HE—AAC类型音频播放速率减半的问题
v114.14.8 更新：
	xc最小化之后白屏问题
	解决崩溃问题
	解决商业QT焦点崩溃问题
v114.14.7 更新：
	针对关于主窗口未创建cef，子窗创建cef，关闭子窗时导致主窗退出(如扫码登录场景)修改更新:
	新增 testdialog对话框类测试，并修改mainwindow，newwindow类
	移除 TCSimpleHandler冗余构造
	删除 EntryAbility.ets 中 WindowManager.setWindowStage 的设置，并修改继承关系 WebQtAbility
	修改 web_engine\Index.ets 导入 WebQtAbility
	新增 WebQtAbility.ets 供qt+cef使用
	恢复原 cef的EntryAbility.ets，【后修改为WebEntryAbilityPopup.ets】，并重命名 EntryAbilityPopup.ets 供原cef使用
	移除 web_engine\src\main\module.json5 中 WebEntryAbilityPopup.ets 入口信息，并将重命名后的 EntryAbilityPopup.ets，添加到 entry\src\main\module.json5
	恢复原 WebAbility 和 WebBaseAbility.ets 相关逻辑
	AbilityManager.ets 添加针对子对话框通过xcid获取相关窗口和UIAbility信息
	标题、光标、拖拽、右键菜单相关功能，修改相关文件：AppWindowAdapter.ets、CursorAdapter.ets、DragDropAdapter.ets、SubWindowAdapter.ets
	
	【注：主要更新内容是QT+CEF和原CEF的Ability分离，内核代码不需修改】
	
V114.14.6 更新：
	修改禁止dock栏--右键--打开新的窗口
	删掉SYSTEM_FLOAT_WINDOW和ALLOW_WRITABLE_CODE_MEMORY
	优化焦点抢焦问题
	修复删除节点崩溃,离屏节点判断,
	引入chromium_25_12_19.patch
	引入chromium_25_11_28.patch
	引入chromium_25_12_19.patch
	引入chromium_26_01_05.patch
V114.14.5 更新：
	Ctrl + 鼠标滚轮放大缩小
	引入chromium_25_11_07.patch
	引入chromium_25_11_14.patch
 V114.14.4 更新：
	修复焦点抢焦问题
	提供一个新接口，让应用可以获取额外的系统信息
	修复在num_lock灯灭掉的时候数字小键盘按键7功能不生效的问题
  V114.14.3 更新：
	引入chromium_25_10_10.patch
	引入chromium_25_09_22.patch
	引入chromium_25_09_15.patch
	修复cef网页内拖拽drop事件缺失的问题
	修复共享内存关闭崩溃问题
 V114.14.2 更新：
	引入chromium_25_10_10.patch
	引入chromium_25_09_22.patch
	引入chromium_25_09_15.patch
	修复cef网页内拖拽drop事件缺失的问题
	修复共享内存关闭崩溃问题	

  V114.14.2 更新：
	引入chromium_25_09_05.patch
	优化新窗口创建功能实现
	优化WebAbility，EntryAbility，WebEntryAbility实现
	解决CAJ鼠标样式变化时崩溃问题
  
  v114.14.1 更新：
	引入patch：chromium_25_08_29.patch
  
  v114.14.0 更新：
    修复QT子窗口中tooltip渲染问题
    引入patch：chromium_25_08_15.patch，chromium_25_08_08.patch，chromium_25_08_01.patch，chromium_25_07_25.patch

  v_20250806_1 更新：
	添加F12开发者工具弹窗
	添加按钮点击打开新窗口功能
	添加按钮打开QT子窗口功能
	修复QT子窗口鼠标样式变化功能

  v_20250731_1 更新：
	any-hover需求开发
	修复点击百度打开新窗口后崩溃问题

  v_20250718_1 更新:
	切api17
	调整浏览器关闭流程（该patch恢复：qt和cefability分离，创建cef需要）
	引入patch：chromium_25_07_11.patch, DRM相关patch补入（0513,0516,0606 patch忽略DRM）
	适配CEF对应DRM功能
	修复窗口标题
	修复adapter_c崩溃场景
	测试弹窗: qt和cef创建abilit分离【v_20250716_1】，修复拖拽bug
	提交qt+cef工程entry代码文件

  v_20250716_1 更新:
	测试弹窗: qt主窗口上内嵌cef，cef作为qt的子窗【qt子窗口无ability实例，导致cef无句柄】；其他为纯cef窗口，如点击超链接新建弹窗

  v_20250710_1 更新:
	引入patch：chromium_25_07_04.patch,cef_asan.patch
	
  v_20250628_1 更新:
	引入patch：关于CEF性能,chromium_25_06_20.patch,chromium_25_06_28.patch

  v_20250620_2 更新:
	开启accessibility无障碍功能
	
  v_20250619_1 更新:
	引入patch: chromium_25_05_13.patch,chromium_25_05_16.patch,chromium_25_05_20.patch,chromium_25_05_23.patch,chromium_25_05_28.patch,chromium_25_06_06.patch,chromium_25_06_16.patch
	添加jit权限检测

  v_20250509_1 更新:
	回退提交0ae28bc45444389d9b5ec00bc7205be4347d2544和11ec62d3866ccedd758e31f954992816b3e09af3
	修改bing.com站点,权限拒绝后,语音输入UI提示仍在监听输入问题

  v_20250508_1 更新:
	解决关闭延时问题
	*** 更新QT框架 ***(Qt5.15.12_arm64_5.0.3.135-15_20250427.win)
	引入xrm_patch.patch：解决拖动导致的崩溃问题patch 
	引入chromium_25_04_30.patch,114新增修改：
		1、【功能-bugfix】修复多窗口情况下，地理位置权限请求可能会再其他窗口出现的问题 2、【功能--BUGFIX】修复拖拽标签页时按下ESC后标签页拖拽流程没有结束的问题 3、performance: on background lower qos in ohos
	解决系统版本115获取触控板类型为空，导致抛滑失效问题
	引入chromium_25_04_27.patch，新增修改：
		1、[功能-Bugfix] 修复拖拽标签页合入拖出，编辑光标不恢复问题 2、【性能】optimize resSched point and optimize throttle time
	添加右键菜单查看源码，优化代码
  v_20250423_2 更新:
	修复patch引入XComponent重构问题，还原QT+CEF中OnSurfaceChanged事件
	修改下载路径默认为App路径
	优化获取XComponent的id
	修复patch引入XComponent重构问题，还原QT+CEF中OnKeyEventCB事件
  v_20250423_1 更新：
	引入chromium_25_04_18.patch，新增修改：
		1、【功能--BUGFIX】修复通过自定义及控制Chromium退出浏览器时出现要恢复页面吗弹窗的问题 2、【稳定性】修改内存分配模块打日志造成的编译问题 3、【稳定性】解决关闭窗口对应XComponentImpl实例未被析构而导致的白屏问题 4、【稳定性】XComponent回调管理优化重构 5、【功能--BUGFIX】修复右键docker栏浏览器图标点击打开新的窗口后实际没有打开新窗口问题 6、【功能--BUGFIX】剪贴板权限请求切换同步请求，修复首次复制粘贴失败问题；去除鼠标中间粘贴事件 7、【功能--BUGFIX】修复任务管理器打开显示异常问题 8、【功能--BUGFIX】解决快速频繁切换两个窗口的输入框有概率输入法失效的问题
	修改输入法显示错误，未跟随光标
	临时patch，添加Trace
	修改坚盾模式，开启该模式时CEF内核处理为禁用，即添加--jitless
  v_20250416_2 更新：更新系统分辨率缩放比获取
  v_20250416_1 更新：
	更新demo中EWAdapterC对像素缩放的相关修改;
	修改窗口页面旋转问题;
	修改下拉框错位问题
	引入chromium_25_04_11.patch，新增修改：
		1、【功能--BUGFIX】修复摄像头预览画面色彩异常问题 2、【功能--BUGFIX】接入下载控件，修复下载首次不生效问题 3、【功能--BUGFIX】修复多次打开设置-外观时，字体列表中字体重复显示问题 4、【功能】传感器抽象类提取 5、【功能--BUGFIX】修复设置默认浏览器时未拉起系统默认浏览器设置页面问题 6、【功能--BUGFIX】修复触摸拖拽标签页时合入目标窗口失败的问题 7、【功能--BUGFIX】修复长按最大化按钮响应右键问题
	引入chromium_25_04_03.patch，新增修改： 
		1、【安全】CVE补丁合入 CVE-2025-24201 2、【功能--BUGFIX】调整浏览器关闭流程 3、【性能-Bugfix】add res sched module and boost freq with gesture 添加提频的接口以及滑动时进行提频 4、【功能--BUGFIX】修复标签页拖拽在扩展屏中合入目标窗口失败的问题 5、【功能-Bugfix】 Fixed the issue that the right-click file in the gallery cannot be opened using Chromium 6、【功能-BUGFIX】解决Asan功能不可用问题 7、【功能--BUGFIX】修复跨应用拖拽不支持多entry的问题
  v_20250402_1 更新qt框架和工程结构（内核和v_20250328_1一致）
  v_20250328_1 更新250326-patch,修复拖拽错位,菜单错位,tab按键事件
  v_20250327_1 更新250321-patch,修复拖拽
  v_20250320_1 更新SDK,适配触控板双指抛滑、捏合页面缩放
  v_20250314_1 适配触控板捏合页面缩放临时方案
  ...

  
  
  
  
  
  
  
!!!注意:对应API15,需要升级SDK版本至 5.0.3.135

***DevEco版本***:	DevEco Studio for Windows 5.0.9.300
***SDK版本******:	Command Line Tools for Linux 5.0.9.300
***系统版本*****:	HYM—W5821 205.0.0.308（SP37DEVC00E80R1P30log）

2025-03-20
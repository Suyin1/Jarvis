cmake_minimum_required(VERSION 3.5.0)
project(OpenCefArkTs)

set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include
                    ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c)

# libcef_dll_wrapper
set_property(GLOBAL PROPERTY OS_FOLDERS ON)
set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")
find_package(CEF REQUIRED)
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

find_library(native-drawing-lib native_drawing)
find_library(native-window-lib native_window)
find_library(hilog-lib hilog_ndk.z)
find_library(libace-lib ace_ndk.z)
find_library(libnapi-lib ace_napi.z)

# ============================================================
# libcefsimple.so — CEF 核心库
# 包含 Handler/App/Render/V8 扩展/JsCallAPI 等
# ============================================================
add_library(cefsimple SHARED
    testcef/StringUtil.cpp
    testcef/TCSimpleApp.cpp
    testcef/TCSimpleCefApp.cpp
    testcef/TCSimpleHandler.cpp
    testcef/TCSimpleRender.cpp
    testcef/CefSimpleOhos.cpp
)

# Determine native libs path based on target ABI
if (CMAKE_ANDROID_ARCH_ABI STREQUAL "x86_64")
    set(NATIVE_LIBS_PATH ${NATIVERENDER_ROOT_PATH}/../../../libs/x86_64)
else()
    set(NATIVE_LIBS_PATH ${NATIVERENDER_ROOT_PATH}/../../../libs/arm64-v8a)
endif()

target_link_libraries(cefsimple PUBLIC
    ${NATIVE_LIBS_PATH}/libcef.so
    libcef_dll_wrapper
    ${NATIVE_LIBS_PATH}/libadapter.so
    ${native-drawing-lib}
    ${native-window-lib}
    ${hilog-lib}
    ${libace-lib}
    ${libnapi-lib}
)

# ============================================================
# libadapter_c.so — ArkUI 原生节点适配器 + CEF 桥接管理
# 提供 FrameNode 管理、NAPI 导出、桥接函数指针、
# CEF 初始化与浏览器生命周期管理（替换原 mainwindow 角色）
# ============================================================
add_library(adapter_c SHARED
    ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c/adapter_c.cpp
    ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c/adapter_c.h
    ${NATIVERENDER_ROOT_PATH}/cef_bridge/CefBridge.cpp
    ${NATIVERENDER_ROOT_PATH}/cef_bridge/CefBridge.h
)

target_link_libraries(adapter_c PUBLIC
    cefsimple
    ${hilog-lib}
    libace_napi.z.so
    uv
    ${libace-lib}
    ${native-window-lib}
)

# the minimum version of CMake.
cmake_minimum_required(VERSION 3.5.0)
project(OpenQtCefDebug)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})

include_directories(${NATIVERENDER_ROOT_PATH}
                    ${NATIVERENDER_ROOT_PATH}/include
                    ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c)


# libcef_dll_wrapper 编译
# Use folders in the resulting project files.
set_property(GLOBAL PROPERTY OS_FOLDERS ON)

set(CEF_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CEF_ROOT}/cmake")

find_package(CEF REQUIRED)

# Include the libcef_dll_wrapper target.
# Comes from the libcef_dll/CMakeLists.txt file in the binary distribution
# directory.
add_subdirectory(${CEF_LIBCEF_DLL_WRAPPER_PATH} libcef_dll_wrapper)

find_library(
    # Sets the name of the path variable.
    native-drawing-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    native_drawing
)

find_library(
    # Sets the name of the path variable.
    native-window-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    native_window
)

find_library(
    # Sets the name of the path variable.
    hilog-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    hilog_ndk.z
)

find_library(
    # Sets the name of the path variable.
    libace-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    ace_ndk.z
)

find_library(
    # Sets the name of the path variable.
    libnapi-lib
    # Specifies the name of the NDK library that
    # you want CMake to locate.
    ace_napi.z
)

add_library(cefsimple SHARED
    testcef/StringUtil.cpp
    testcef/TCSimpleApp.cpp
    testcef/TCSimpleCefApp.cpp
    testcef/TCSimpleHandler.cpp
    testcef/TCSimpleRender.cpp
    testcef/CefSimpleOhos.cpp
)

add_library(adapter_c SHARED ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c/adapter_c.h ${NATIVERENDER_ROOT_PATH}/ohos/adapter_c/adapter_c.cpp)
target_link_libraries(adapter_c PUBLIC ${hilog-lib} libace_napi.z.so uv ${libace-lib})

target_link_libraries(cefsimple PUBLIC
    ${NATIVERENDER_ROOT_PATH}/../../../libs/arm64-v8a/libcef.so
    libcef_dll_wrapper
    ${NATIVERENDER_ROOT_PATH}/../../../libs/arm64-v8a/libadapter.so
    ${native-drawing-lib}
    ${native-window-lib}
    ${hilog-lib}
    ${libace-lib}
    ${libnapi-lib}
)

list(APPEND CMAKE_FIND_ROOT_PATH ${QT_PREFIX})
add_library(qtmodule SHARED main.cpp mainwindow/mainwindow.cpp mainwindow/newwindow.cpp)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets)

target_link_libraries(qtmodule PRIVATE
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
    adapter_c
    cefsimple
)


#set(NATIVERENDER_ROOT_PATH ${CMAKE_CURRENT_SOURCE_DIR})
#
#if(DEFINED PACKAGE_FIND_FILE)
#    include(${PACKAGE_FIND_FILE})
#endif()
#
#include_directories(${NATIVERENDER_ROOT_PATH}
#                    ${NATIVERENDER_ROOT_PATH}/include)
#
#add_library(entry SHARED napi_init.cpp)
#target_link_libraries(entry PUBLIC libace_napi.z.so)
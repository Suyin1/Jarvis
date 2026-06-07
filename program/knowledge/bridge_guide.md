# QT ↔ ArkTS NAPI Bridge Development Guide

## Overview

This document is the canonical reference for building hybrid applications that use QT (C++) for backend logic and ArkTS (HarmonyOS) for UI, connected through a NAPI bridge layer.

## Architecture

```
┌──────────────────┐     NAPI Calls      ┌──────────────────┐     QT API      ┌──────────────┐
│   ArkTS UI       │ ◄────────────────►  │  NAPI Bridge     │ ◄────────────► │  QT C++ Logic │
│  (HarmonyOS)     │    JSON/raw data     │  (C++ wrapper)   │                │  (Backend)    │
└──────────────────┘                     └──────────────────┘                └──────────────┘
```

### Data Flow

1. ArkTS calls `nativeModule.methodName(args)` 
2. NAPI bridge receives the call, validates args, and forwards to QT
3. QT processes the request and returns results
4. NAPI bridge converts the result back to JS types
5. ArkTS receives the response (sync or Promise-based)

## NAPI Bridge Implementation

### Required Headers

```cpp
#include <napi.h>
#include <string>
#include <thread>
#include <functional>
```

### Standard Bridge Class Pattern

```cpp
class MyBridge : public Napi::ObjectWrap<MyBridge> {
public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "MyBridge", {
            InstanceMethod("processText", &MyBridge::ProcessText),
            InstanceMethod("asyncProcess", &MyBridge::AsyncProcess),
        });

        auto constructor = Napi::Persistent(func);
        constructor.SuppressDestruct();
        exports.Set("MyBridge", func);
        return exports;
    }

    MyBridge(const Napi::CallbackInfo &info)
        : Napi::ObjectWrap<MyBridge>(info) {
        // Initialize QT module
        qtModule_ = new MyQtModule();
    }

    ~MyBridge() {
        delete qtModule_;
    }

private:
    MyQtModule *qtModule_;

    Napi::Value ProcessText(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        if (info.Length() < 1 || !info[0].IsString()) {
            Napi::TypeError::New(env, "String argument expected").ThrowAsJavaScriptException();
            return env.Null();
        }

        std::string input = info[0].As<Napi::String>().Utf8Value();
        std::string result = qtModule_->processText(input);

        return Napi::String::New(env, result);
    }

    Napi::Value AsyncProcess(const Napi::CallbackInfo &info) {
        Napi::Env env = info.Env();
        // ... AsyncWorker pattern for non-blocking calls
    }
};

// Module registration
Napi::Object InitAll(Napi::Env env, Napi::Object exports) {
    return MyBridge::Init(env, exports);
}
NODE_API_MODULE(my_module, InitAll)
```

### Thread Safety

NAPI calls may arrive from any thread. Follow these rules:
- QT GUI methods MUST be called from the main QT thread
- Use `QMetaObject::invokeMethod()` with `Qt::QueuedConnection` for cross-thread calls
- Use `napi_call_threadsafe_function` for JS callbacks from worker threads

### Type Conversion Reference

| C++ Type | NAPI Type | Conversion Code |
|----------|-----------|-----------------|
| `std::string` | `Napi::String` | `info[0].As<Napi::String>().Utf8Value()` |
| `int32_t` | `Napi::Number` | `info[0].As<Napi::Number>().Int32Value()` |
| `double` | `Napi::Number` | `info[0].As<Napi::Number>().DoubleValue()` |
| `bool` | `Napi::Boolean` | `info[0].As<Napi::Boolean>().Value()` |
| `std::vector` | `Napi::Array` | Iterate and push elements |
| `std::map` | `Napi::Object` | Set key-value pairs on object |

## ArkTS Side

### Importing Native Modules

```typescript
// The .so library name must match the NODE_API_MODULE macro
import myModule from 'libmymodule.so';

// For class-based bridges, create an instance
const bridge = new myModule.MyBridge();

// Call methods
const result: string = bridge.processText('hello');
```

### Error Handling Pattern

```typescript
try {
    const result = await bridge.asyncProcess(input);
    // Handle success
} catch (error) {
    console.error(`Native module error: ${error}`);
    // Show user-facing error
}
```

### Common Error Patterns and Solutions

| Error | Cause | Fix |
|-------|-------|-----|
| `Cannot find module` | .so not in correct path | Check libs/arm64-v8a/ placement |
| `symbol not found` | Method not exported | Verify NODE_API_MODULE and DefineClass |
| `TypeError: not a function` | Wrong import style | Use `new` for class-based bridges |
| Segfault in bridge | Memory ownership issue | Ensure C++ object lifetime matches JS |

## CMake Configuration

### Output Structure

```
# NAPI module → Shared Library
add_library(my_module SHARED bridge.cpp)
target_link_libraries(my_module PRIVATE ${NAPI_LIBRARIES})

# QT app → Executable
add_executable(my_app main.cpp qt_module.cpp)
target_link_libraries(my_app PRIVATE Qt6::Core Qt6::Widgets)
```

## Build & Deployment

1. Build NAPI module: `cmake --build . --target my_module`
2. Build QT app: `cmake --build . --target my_app`  
3. For HarmonyOS: Copy .so to `entry/libs/arm64-v8a/`
4. Package with `hvigorw assembleHap`

## Testing

- Unit test QT logic independently (no NAPI dependency)
- Test NAPI bridge with simple ArkTS test page
- End-to-end: automated UI test that triggers flow and verifies output

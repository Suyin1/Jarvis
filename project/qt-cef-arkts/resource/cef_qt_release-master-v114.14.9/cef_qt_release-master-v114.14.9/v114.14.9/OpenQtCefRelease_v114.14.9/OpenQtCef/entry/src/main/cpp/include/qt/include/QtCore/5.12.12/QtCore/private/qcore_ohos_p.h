/****************************************************************************
**
** Copyright (C) 2024 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCORE_OHOS_P_H
#define QCORE_OHOS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qcore_ohos_internalwindowid_p.h>
#include <QtCore/private/qnapi_p.h>
#include <QtCore/qglobal.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>
#include <algorithm>
#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <string>
#include <typeinfo>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

template<typename Enum>
struct OhosEnumMeta
{
};

class Q_CORE_EXPORT QObjectThreadSafeRef
{
public:
    QObjectThreadSafeRef();
    QObjectThreadSafeRef(QPointer<QObject> obj);

    QObjectThreadSafeRef(const QObjectThreadSafeRef &other);
    QObjectThreadSafeRef &operator=(const QObjectThreadSafeRef &other);

    bool operator==(const QObjectThreadSafeRef &other) const;
    bool operator!=(const QObjectThreadSafeRef &other) const;

    std::string refName() const;

    // it may be called only inside Qt thread that created the reference
    QPointer<QObject> data() const;

private:
    struct QObjectRef
    {
        QPointer<QObject> obj;
        std::string refName;
    };

    static std::mutex refsMapMutex;
    static std::uint64_t refsMapInsertCounter;
    static std::map<QObject *, std::shared_ptr<QObjectRef>> refsMap;

    pthread_t m_creatorThread;
    std::string m_refName;
    std::weak_ptr<QObjectRef> m_weakObjRef;
};

class Q_CORE_EXPORT QAbilityPeer
{
public:
    virtual ~QAbilityPeer();

    // TODO: wrappers for backward-compatibility only, remove when no longer needed
    std::string getInstanceId() { return instanceId(); }
    QNapi::Object getQAbility() { return qAbility(); }
    std::shared_ptr<std::atomic_bool> getDestroyAllowedFlag() { return destroyAllowedFlag(); }

    virtual std::string instanceId() = 0;
    virtual QNapi::Object qAbility() = 0;
    virtual QNapi::Object windowStage() = 0;
    virtual QNapi::Object windowManager() = 0;
    virtual QObjectThreadSafeRef qWindowRef() = 0;
    virtual std::shared_ptr<std::atomic_bool> destroyAllowedFlag() = 0;

    virtual void setQWindow(QObjectThreadSafeRef qwindow) = 0;

protected:
    QAbilityPeer();
};

class Q_CORE_EXPORT AppFunctions
{
public:
    virtual ~AppFunctions();

    virtual void startQAbilityInstance(
        QNapi::Object baseQAbility, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) = 0;

    virtual void startAppProcess(
        QNapi::Object baseQAbility, const std::string &processId, QNapi::Object want,
        QNapi::Object optStartOptions) = 0;

    virtual void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) = 0;
};

class JsState
{
public:
    JsState(const JsState &) = delete;
    JsState &operator=(const JsState &) = delete;

    // TODO: wrappers for backward-compatibility only, remove when no longer needed
    napi_env getEnv() { return env(); };
    std::shared_ptr<QAbilityPeer> getQAbilityPeer() { return defaultQAbilityPeer(); }
    QNapi::Object getAppLaunchWant() { return appLaunchWant(); }

    virtual ~JsState();

    virtual napi_env env() = 0;
    virtual QNapi::Object getModule(const std::string &moduleName) = 0;

    virtual QNapi::Object appLaunchWant() = 0;

    virtual std::shared_ptr<QAbilityPeer> defaultQAbilityPeer() = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstanceId(const std::string &instanceId) = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByInstance(QNapi::Object qAbility) = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByQWindow(QObjectThreadSafeRef qwindow) = 0;
    virtual std::shared_ptr<QAbilityPeer> tryGetQAbilityPeerByWindowId(InternalWindowId windowId) = 0;

    virtual void registerWindow(QObjectThreadSafeRef qwindow, InternalWindowId windowId) = 0;

    virtual void startNewQAbilityInstance(
        std::shared_ptr<QAbilityPeer> baseQAbilityPeer, QObjectThreadSafeRef qwindow,
        QNapi::Object optStartOptions,
        std::function<void(std::shared_ptr<QAbilityPeer>)> startupNotifyFunc) = 0;

    virtual void startAppProcess(
        const std::string &processId, QNapi::Object requestWant,
        QNapi::Object optStartOptions = {}) = 0;

    virtual void addNewWantConsumer(std::function<void(QNapi::Object)> wantConsumer) = 0;

    virtual void startNoUiChildProcess(const std::string &libraryName, const std::vector<std::string> &args) = 0;

    template<typename Enum>
    QNapi::Number mapOhosEnumToJs(Enum enumValue);

    template<typename Enum>
    Enum mapOhosEnumFromJs(QNapi::Number enumJsValue);

protected:
    struct OhosEnumInfo
    {
        std::string moduleName;
        std::string typeName;
        std::vector<std::pair<int, const char *>> enumeratorsNames;
    };

    JsState();

private:
    template<typename Enum>
    static OhosEnumInfo makeOhosEnumInfo();

    virtual QNapi::Number mapOhosEnumToJs(int enumValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) = 0;
    virtual int mapOhosEnumFromJs(QNapi::Number enumJsValue, const std::type_info &enumTypeInfo, OhosEnumInfo (*ohosEnumInfoFactory)()) = 0;
};

// this function should be called once from JS thread at some point during startup
Q_CORE_EXPORT void initJsThreadState(
    napi_env env, std::map<std::string, Napi::Reference<QNapi::Object>> &&jsModules,
    std::shared_ptr<AppFunctions> appFunctions);

// this function should be called from JS thread for each UIAbility when it's ready
Q_CORE_EXPORT void addJsQAbilityPeer(std::shared_ptr<QAbilityPeer> qAbilityPeer);

// this function should be called from JS thread when WindowStage of UIAbility is destroyed
Q_CORE_EXPORT void removeMatchingJsQAbilityPeer(QNapi::Object qAbility);

// this function should be called from JS thread when new Want object is received
Q_CORE_EXPORT void dispatchNewWant(QNapi::Object want);

// invokes the task inside the JS thread, can be called from Qt thread at any time
Q_CORE_EXPORT void invokeInJsThread(std::function<void(JsState &)> task);

// calls the function synchronously with JsState arg, can be called only from JS thread
Q_CORE_EXPORT void runWithJsState(const std::function<void(JsState &)> &func);

template<typename Func>
auto evalWithJsState(Func &&func) -> decltype(func(std::declval<JsState &>()));

// Invokes the task inside the JS thread and blocks the caller's thread until
// the "continue" function (std::function<void()>, passed as second argument to
// the task) is called on the JS side.
// It can be called from the Qt thread at any time, calling it from the JS
// thread is illegal.
Q_CORE_EXPORT void invokeInJsThreadAndWaitForContinue(
    std::function<void(JsState &, std::function<void()>)> &&task);

// Runs the task inside the JS thread and waits until its execution ends.
// When called from the JS thread, it calls the task directly. For other threads
// it behaves like a wrapper around the invokeInJsThreadAndWaitForContinue().
Q_CORE_EXPORT void runInJsThreadAndWait(const std::function<void(JsState &)> &task);

// this function should be called once from Qt thread at some point during startup
Q_CORE_EXPORT void initQtThreadState();

template<typename Func>
auto evalInJsThread(Func &&func) -> decltype(func(std::declval<JsState &>()));

// invokes the task inside the Qt thread, can be called from any thread at any time
Q_CORE_EXPORT void invokeInQtThread(std::function<void()> task);

template<typename Func>
auto evalWithJsState(Func &&func) -> decltype(func(std::declval<JsState &>()))
{
    using Result = decltype(func(std::declval<JsState &>()));
    Result result;
    runWithJsState(
        [&](JsState &jsState) {
            result = func(jsState);
        });
    return result;
}

template<typename Func>
auto evalInJsThread(Func &&func) -> decltype(func(std::declval<JsState &>()))
{
    using Result = decltype(func(std::declval<JsState &>()));
    Result result;
    runInJsThreadAndWait(
        [&](JsState &jsState) {
            result = func(jsState);
        });
    return result;
}

template<typename Enum>
QNapi::Number JsState::mapOhosEnumToJs(Enum enumValue)
{
    return mapOhosEnumToJs(static_cast<int>(enumValue), typeid(Enum), &makeOhosEnumInfo<Enum>);
}

template<typename Enum>
Enum JsState::mapOhosEnumFromJs(QNapi::Number enumJsValue)
{
    return static_cast<Enum>(mapOhosEnumFromJs(enumJsValue, typeid(Enum), &makeOhosEnumInfo<Enum>));
}

template<typename Enum>
JsState::OhosEnumInfo JsState::makeOhosEnumInfo()
{
    static const auto enumEnumeratorsNames = OhosEnumMeta<Enum>::enumeratorsNames;

    std::vector<std::pair<int, const char *>> intEnumeratorsNames;
    std::transform(
        enumEnumeratorsNames.begin(), enumEnumeratorsNames.end(),
        std::back_inserter(intEnumeratorsNames),
        [](const auto &valueNamePair) {
            return std::pair<int, const char *>(static_cast<int>(valueNamePair.first), valueNamePair.second);
        });

    return OhosEnumInfo {
        .moduleName = OhosEnumMeta<Enum>::moduleName,
        .typeName = OhosEnumMeta<Enum>::typeName,
        .enumeratorsNames = std::move(intEnumeratorsNames),
    };
}

}

QT_END_NAMESPACE

#endif

/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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

#ifndef QNAPI_P_H
#define QNAPI_P_H

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

#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qstring.h>
#include <algorithm>
#include <array>
#include <functional>
#include <map>
#include <memory>
#include <napi.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QNapi {

class Object;
class Promise;

class CallbackFuncWrapper
{
public:
    template<typename Func>
    CallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(const Napi::CallbackInfo &)>>::value, char *> = nullptr);

    template<typename Func>
    CallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(const Napi::CallbackInfo &)>>::value, short *> = nullptr);

    template<typename Func>
    CallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func()>>::value, int *> = nullptr);

    template<typename Func>
    CallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func()>>::value, long *> = nullptr);

    std::function<Napi::Value(const Napi::CallbackInfo &)> &callbackFunc();

private:
    std::function<Napi::Value(const Napi::CallbackInfo &)> m_callbackFunc;
};

namespace details_qnapi_p_h {

template<typename T>
struct ValueTypeTraits
{
};

template<>
struct ValueTypeTraits<Napi::Boolean>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBoolean;
    static constexpr const char *typeName = "Boolean";
};

template<>
struct ValueTypeTraits<Napi::Number>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsNumber;
    static constexpr const char *typeName = "Number";
};

template<>
struct ValueTypeTraits<Napi::BigInt>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBigInt;
    static constexpr const char *typeName = "BigInt";
};

template<>
struct ValueTypeTraits<Napi::Date>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsDate;
    static constexpr const char *typeName = "Date";
};

template<>
struct ValueTypeTraits<Napi::String>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsString;
    static constexpr const char *typeName = "String";
};

template<>
struct ValueTypeTraits<Napi::Symbol>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsSymbol;
    static constexpr const char *typeName = "Symbol";
};

template<>
struct ValueTypeTraits<Napi::Array>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsArray;
    static constexpr const char *typeName = "Array";
};

template<>
struct ValueTypeTraits<Napi::ArrayBuffer>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsArrayBuffer;
    static constexpr const char *typeName = "ArrayBuffer";
};

template<>
struct ValueTypeTraits<Napi::TypedArray>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsTypedArray;
    static constexpr const char *typeName = "TypedArray";
};

template<>
struct ValueTypeTraits<Napi::Object>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsObject;
    static constexpr const char *typeName = "Object";
};

template<>
struct ValueTypeTraits<Object> : public ValueTypeTraits<Napi::Object>
{
};

template<>
struct ValueTypeTraits<Napi::Function>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsFunction;
    static constexpr const char *typeName = "Function";
};

template<>
struct ValueTypeTraits<Napi::Promise>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsPromise;
    static constexpr const char *typeName = "Promise";
};

template<>
struct ValueTypeTraits<Promise> : public ValueTypeTraits<Napi::Promise>
{
};

template<>
struct ValueTypeTraits<Napi::DataView>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsDataView;
    static constexpr const char *typeName = "DataView";
};

template<typename BufferT>
struct ValueTypeTraits<Napi::Buffer<BufferT>>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsBuffer;
    static constexpr const char *typeName = "Buffer";
};

template<typename ExternalT>
struct ValueTypeTraits<Napi::External<ExternalT>>
{
    static constexpr auto typeCheckMemFun = &Napi::Value::IsExternal;
    static constexpr const char *typeName = "External";
};

template<typename T>
constexpr bool isCallbackFuncType()
{
    return std::is_constructible<CallbackFuncWrapper, T>::value;
}

template<typename T>
inline std::enable_if_t<isCallbackFuncType<T>(), Napi::Value> makeValue(napi_env env, T &&inputValue)
{
    return Napi::Function::New(env, std::move(CallbackFuncWrapper(std::forward<T>(inputValue)).callbackFunc()));
}

template<typename T>
inline std::enable_if_t<!isCallbackFuncType<T>(), Napi::Value> makeValue(napi_env env, T &&inputValue)
{
    return Napi::Value::From(env, std::forward<T>(inputValue));
}

inline Napi::Error makeLoggedExceptionImpl(napi_env env, const std::string &msg)
{
    qOhosCritical(QtForOhos) << "QNapi exception: " << QString::fromUtf8(msg.c_str());
    return Napi::Error::New(env, msg);
}

template<typename T>
std::enable_if_t<std::is_same<T, Napi::Value>::value, bool> valueTypeMatchesImpl(const Napi::Value &)
{
    return true;
}

template<typename T>
std::enable_if_t<!std::is_same<T, Napi::Value>::value, bool> valueTypeMatchesImpl(const Napi::Value &value)
{
    return (value.*ValueTypeTraits<T>::typeCheckMemFun)();
}

std::string getArrayElementValueTypeString(const Napi::Array &arrayValue);

inline std::string getValueTypeStringImpl(const Napi::Value &value)
{
    using namespace std::string_literals;

    if (value.IsArray()) {
        auto arrayElemTypeString = getArrayElementValueTypeString(value.As<Napi::Array>());
        return "Array<"s + arrayElemTypeString + ">"s;
    }

    // put subtypes of the Napi::Object at the beginning, so that we find most specific type name
    const std::pair<bool (Napi::Value::*)() const, const char *> typesChecksAndNames[] = {
        {&Napi::Value::IsEmpty, "<empty>"},
        {&Napi::Value::IsNull, "Null"},
        {&Napi::Value::IsUndefined, "Undefined"},
        {&Napi::Value::IsArray, "Array"},
        {&Napi::Value::IsArrayBuffer, "ArrayBuffer"},
        {&Napi::Value::IsTypedArray, "TypedArray"},
        {&Napi::Value::IsDataView, "DataView"},
        {&Napi::Value::IsFunction, "Function"},
        {&Napi::Value::IsPromise, "Promise"},
        {&Napi::Value::IsBigInt, "BigInt"},
        {&Napi::Value::IsBuffer, "Buffer"},
        {&Napi::Value::IsBoolean, "Boolean"},
        {&Napi::Value::IsDate, "Date"},
        {&Napi::Value::IsExternal, "External"},
        {&Napi::Value::IsNumber, "Number"},
        {&Napi::Value::IsObject, "Object"},
        {&Napi::Value::IsString, "String"},
        {&Napi::Value::IsSymbol, "Symbol"},
    };

    const char *foundTypeName = nullptr;
    for (const auto &typeCheckEntry : typesChecksAndNames) {
        if ((value.*(typeCheckEntry.first))()) {
            foundTypeName = typeCheckEntry.second;
            break;
        }
    }

    return foundTypeName != nullptr ? foundTypeName : "?";
}

inline std::string getArrayElementValueTypeString(const Napi::Array &arrayValue)
{
    constexpr std::size_t maxArrayElementsForTypeCheck = 10;

    std::size_t arrayLength = arrayValue.Length();
    auto checkRangeSize = std::min(arrayLength, maxArrayElementsForTypeCheck);

    std::string commonElemTypeString;
    for (std::size_t i = 0; i < checkRangeSize; ++i) {
        auto elemTypeString = getValueTypeStringImpl(arrayValue.Get(i));
        if (commonElemTypeString.empty()) {
            commonElemTypeString = elemTypeString;
        } else if (commonElemTypeString != elemTypeString) {
            commonElemTypeString = "";
            break;
        }
    }

    return !commonElemTypeString.empty()
        ? checkRangeSize == arrayLength
            ? commonElemTypeString
            : commonElemTypeString + "?"
        : "?";
}

template<typename T, typename ValueDescriptionSupplier>
std::enable_if_t<std::is_same<T, Napi::Value>::value, T> checkedCastImpl(
    const Napi::Value &value, ValueDescriptionSupplier &&)
{
    return value;
}

template<typename T, typename ValueDescriptionSupplier>
std::enable_if_t<!std::is_same<T, Napi::Value>::value, T> checkedCastImpl(
    const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier)
{
    using namespace std::string_literals;

    if (!valueTypeMatchesImpl<T>(value)) {
        constexpr const char *expectedTypeName = ValueTypeTraits<T>::typeName;
        auto valueTypeStr = getValueTypeStringImpl(value);
        std::string valueDesc = valueDescSupplier();
        auto baseEerrorMsg =
            "wrong type (expected '"s + expectedTypeName + "', got '"s + valueTypeStr + "') of Napi value"s;
        throw makeLoggedExceptionImpl(
            value.Env(), valueDesc.empty() ? baseEerrorMsg : baseEerrorMsg + ": "s + valueDesc);
    }

    return T(value.Env(), value);
}

template<typename Result, typename F>
Result runEscapingHandleScopeImpl(napi_env env, F &&func)
{
    Napi::EscapableHandleScope scope(env);
    Result result = std::forward<F>(func)();
    return checkedCastImpl<Result>(
        scope.Escape(result),
        [&]() {
            return "value escaping from HandleScope";
        });
}

template<typename T>
std::pair<Napi::Object, T> getWithContextImpl(Napi::Object obj, const std::string &expr)
{
    using namespace details_qnapi_p_h;
    using namespace std::string_literals;

    std::pair<Napi::Object, Napi::Value> result;

    auto lastDotPos = expr.rfind('.');

    if (expr.size() >= 2 && expr.substr(expr.size() - 2) == "()") {
        auto subFuncExpr = expr.substr(0, expr.size() - 2);
        auto subFuncWithCtx = getWithContextImpl<Napi::Function>(obj, subFuncExpr);
        auto funcCallResult = !subFuncWithCtx.first.IsEmpty()
            ? subFuncWithCtx.second.Call(subFuncWithCtx.first, {})
            : subFuncWithCtx.second.Call({});

        result = std::make_pair(Napi::Object(), funcCallResult);
    } else if (lastDotPos != std::string::npos) {
        auto subObjExpr = expr.substr(0, lastDotPos);
        auto propName = expr.substr(lastDotPos + 1);

        auto subObj = getWithContextImpl<Napi::Object>(obj, subObjExpr).second;
        if (!subObj.Has(propName)) {
            throw makeLoggedExceptionImpl(
                obj.Env(), "object '"s + subObjExpr + "' has no property named '"s + propName + "'"s);
        }

        result = std::make_pair(subObj, subObj.Get(propName));
    } else {
        if (!obj.Has(expr)) {
            throw makeLoggedExceptionImpl(
                obj.Env(), "object has no property named '"s + expr + "'"s);
        }

        result = std::make_pair(obj, obj.Get(expr));
    }

    return std::make_pair(result.first, checkedCastImpl<T>(result.second, [&]() { return expr; }));
}

inline Napi::Value callMethodWithValueResultImpl(
    const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args)
{
    using namespace std::string_literals;
    return runEscapingHandleScopeImpl<Napi::Value>(
        obj.Env(),
        [&]() {
            auto funcWithCtx = getWithContextImpl<Napi::Function>(obj, methodName);
            auto funcResult = funcWithCtx.second.Call(funcWithCtx.first, args);
            return funcResult;
        });
}

template<typename Result>
Result callMethodImpl(const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args)
{
    using namespace std::string_literals;

    return checkedCastImpl<Result>(
        callMethodWithValueResultImpl(obj, methodName, args),
        [&]() {
            return "result of '"s + methodName + "' method call"s;
        });
}

template<typename Arg>
void getArgImpl(const std::string &funcName, const Napi::CallbackInfo &cbInfo, Arg &arg, std::size_t argIndex)
{
    using namespace std::string_literals;

    arg = checkedCastImpl<Arg>(
        cbInfo[argIndex],
        [&]() {
            return "arg #"s + std::to_string(argIndex) + " of '"s + funcName + "' func call"s;
        });
}

template<typename... Args, std::size_t... Is>
void getLeadingArgsImpl(
    const std::string &funcName, const Napi::CallbackInfo &cbInfo, std::tuple<Args...> args, std::index_sequence<Is...>)
{
    using namespace std::string_literals;

    if (cbInfo.Length() < sizeof...(Args)) {
        throw makeLoggedExceptionImpl(
            cbInfo.Env(),
            "getArgs: func '"s + funcName + "' received less args than expected minimum: "s
            + std::to_string(cbInfo.Length()) + " vs "s + std::to_string(sizeof...(Args)));
    }

    auto unused = {(getArgImpl(funcName, cbInfo, std::get<Is>(args), Is), 0)..., 0};
    (void) unused;
}

}

using Value = Napi::Value;

using Boolean = Napi::Boolean;

using Number = Napi::Number;

using BigInt = Napi::BigInt;

using Date = Napi::Date;

using String = Napi::String;

using Symbol = Napi::Symbol;

using Array = Napi::Array;

using ArrayBuffer = Napi::ArrayBuffer;

using TypedArray = Napi::TypedArray;

using Function = Napi::Function;

using DataView = Napi::DataView;

template<typename BufferT>
using Buffer = Napi::Buffer<BufferT>;

template<typename ExternalT>
using External = Napi::External<ExternalT>;

class ValueWrapper
{
public:
    template<typename T>
    ValueWrapper(T &&inputValue);

    Napi::Value mapToValue(napi_env env) const;

private:
    std::function<Napi::Value(napi_env)> m_valueFactory;
};

class Object : public Napi::Object
{
public:
    using Napi::Object::Object;

    Object(const Napi::Object &other);
    Object &operator=(const Napi::Object &other);

    template<typename T = Value>
    T get(const std::string &expr) const;

    void set(const std::string &name, const ValueWrapper &value);

    void set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues);

    template<typename Result = Value>
    Result call(const std::string &methodName, const std::vector<ValueWrapper> &args = {}) const;
};

class Promise : public Napi::Promise
{
public:
    using Napi::Promise::Promise;

    Promise(const Napi::Promise &other);
    Promise &operator=(const Napi::Promise &other);

    Promise onThen(CallbackFuncWrapper &&onFulfilledFunc);
    Promise onThen(CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc);
    Promise onCatch(CallbackFuncWrapper &&onRejectedFunc);
    Promise onFinally(CallbackFuncWrapper &&onFinallyFunc);
    Promise onThenAndFinally(
        CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc,
        CallbackFuncWrapper &&onFinallyFunc);
};

std::vector<napi_value> unwrapValues(napi_env env, const std::vector<ValueWrapper> &wrappedValues);

Napi::Error makeLoggedException(napi_env env, const std::string &msg);

template<typename T>
bool valueTypeMatches(const Napi::Value &value);

template<typename Element>
bool arrayElementTypesMatch(const Napi::Value &value);

std::string getValueTypeString(const Napi::Value &value);

template<typename T, typename ValueDescriptionSupplier>
T checkedCast(const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier);

template<typename T>
T checkedCast(const Napi::Value &value);

template<typename T = Napi::Value>
T get(const Napi::Object &obj, const std::string &expr);

template<typename T = Napi::Value, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, T>
get(const Napi::Reference<Obj> &objRef, const std::string &expr);

template<typename OutputContainer, typename Element, typename TransFunc>
OutputContainer getArrayElements(const Napi::Array &inputArray, TransFunc &&transFunc);

template<typename OutputContainer, typename Element>
OutputContainer getArrayElements(const Napi::Array &inputArray);

template<typename Result = Napi::Value>
Result callMethod(const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args);

template<typename Result = Napi::Value, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, Result>
callMethod(const Napi::Reference<Obj> &objRef, const std::string &methodName, const std::vector<napi_value> &args);

template<typename Result = Napi::Value>
Result callMethod(const Napi::Object &obj, const std::string &methodName, std::initializer_list<ValueWrapper> args = {});

template<typename Result = Napi::Value, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, Result>
callMethod(const Napi::Reference<Obj> &objRef, const std::string &methodName, std::initializer_list<ValueWrapper> args = {});

Promise callThenOnPromise(
    const Napi::Promise &promise,
    std::function<void(const Napi::CallbackInfo &)> &&onFulfilledFunc,
    std::function<void(const Napi::CallbackInfo &)> &&onRejectedFunc = nullptr);

Promise callThenAndFinallyOnPromise(
    const Napi::Promise &promise,
    std::function<void(const Napi::CallbackInfo &)> &&onFulfilledFunc,
    std::function<void(const Napi::CallbackInfo &)> &&onRejectedFunc,
    std::function<void(const Napi::CallbackInfo &)> &&finallyFunc);

Object makeNewInstance(const Napi::Function &type, const std::vector<ValueWrapper> &args = {});

Object makeNewInstance(const Napi::Object &baseObj, const std::string &typePath, const std::vector<ValueWrapper> &args = {});

template<typename... Args>
void getLeadingArgs(const std::string &funcName, const Napi::CallbackInfo &cbInfo, Args &...args);

template<typename Arg>
Arg getFirstArg(const std::string &funcName, const Napi::CallbackInfo &cbInfo);

template<typename T>
std::shared_ptr<Napi::Reference<T>> makeSharedPersistent(const T &value);

Napi::Value getPropOrUndefined(const Napi::Value &obj, const std::string &propName);

template<typename T>
Napi::Value getPropOrUndefined(const Napi::Reference<T> &objRef, const std::string &propName);

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const std::string &propName, const std::string &objDesc = {});

Object makeObject(napi_env env, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues);

Array makeArray(napi_env env, const std::vector<ValueWrapper> &values);

void setProp(Napi::Object &obj, const std::string &name, const ValueWrapper &value);

void setProps(Napi::Object &obj, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues);

template<typename Result = Napi::Value, typename F>
Result runEscapingHandleScope(napi_env env, F &&func);

template<typename Enum, std::size_t EnumeratorsCount>
std::function<Enum(Napi::Number)> makeFromJsEnumMapper(
    const Napi::Object &enumModule, const std::string &enumName,
    const std::pair<Enum, const char *> (&enumeratorsNames)[EnumeratorsCount]);

template<typename T>
ValueWrapper::ValueWrapper(T &&inputValue)
    : m_valueFactory(
        [inputValue = std::forward<T>(inputValue)](napi_env env) {
            return details_qnapi_p_h::makeValue(env, inputValue);
        })
{
}

inline Napi::Value ValueWrapper::mapToValue(napi_env env) const
{
    return m_valueFactory(env);
}

template<typename Func>
CallbackFuncWrapper::CallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func(const Napi::CallbackInfo &)>>::value, char *>)
    : m_callbackFunc(
        [callbackFunc = std::move(callbackFunc)](const Napi::CallbackInfo &cbInfo) {
            callbackFunc(cbInfo);
            return Napi::Value();
        })
{
}

template<typename Func>
CallbackFuncWrapper::CallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func(const Napi::CallbackInfo &)>>::value, short *>)
    : m_callbackFunc(std::forward<Func>(callbackFunc))
{
}

template<typename Func>
CallbackFuncWrapper::CallbackFuncWrapper(Func &&callbackFunc, std::enable_if_t<std::is_void<std::result_of_t<Func()>>::value, int *>)
    : m_callbackFunc(
        [callbackFunc = std::move(callbackFunc)](const Napi::CallbackInfo &) {
            callbackFunc();
            return Napi::Value();
        })
{
}

template<typename Func>
CallbackFuncWrapper::CallbackFuncWrapper(
    Func &&callbackFunc, std::enable_if_t<std::is_base_of<Napi::Value, std::result_of_t<Func()>>::value, long *>)
    : m_callbackFunc(
        [callbackFunc = std::move(callbackFunc)](const Napi::CallbackInfo &) {
            return callbackFunc();
        })
{
}

inline std::function<Napi::Value(const Napi::CallbackInfo &)> &CallbackFuncWrapper::callbackFunc()
{
    return m_callbackFunc;
}

inline Object::Object(const Napi::Object &other)
    : Napi::Object(other)
{
}

inline Object &Object::operator=(const Napi::Object &other)
{
    Napi::Object::operator=(other);
    return *this;
}

template<typename T>
T Object::get(const std::string &expr) const
{
    return QNapi::get<T>(*this, expr);
}

inline void Object::set(const std::string &name, const ValueWrapper &value)
{
    QNapi::setProp(*this, name, value);
}

inline void Object::set(const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    QNapi::setProps(*this, namedValues);
}

template<typename Result>
Result Object::call(const std::string &methodName, const std::vector<ValueWrapper> &args) const
{
    return QNapi::callMethod<Result>(*this, methodName, unwrapValues(Env(), args));
}

inline Promise::Promise(const Napi::Promise &other)
    : Napi::Promise(other)
{
}

inline Promise &Promise::operator=(const Napi::Promise &other)
{
    Napi::Promise::operator=(other);
    return *this;
}

inline Promise Promise::onThen(CallbackFuncWrapper &&onFulfilledFunc)
{
    return callMethod<Promise>(*this, "then", {std::move(onFulfilledFunc.callbackFunc())});
}

inline Promise Promise::onThen(CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc)
{
    return callMethod<Promise>(
        *this, "then", {std::move(onFulfilledFunc.callbackFunc()), std::move(onRejectedFunc.callbackFunc())});
}

inline Promise Promise::onCatch(CallbackFuncWrapper &&onRejectedFunc)
{
    return callMethod<Promise>(*this, "catch", {std::move(onRejectedFunc.callbackFunc())});
}

inline Promise Promise::onFinally(CallbackFuncWrapper &&onFinallyFunc)
{
    return callMethod<Promise>(*this, "finally", {std::move(onFinallyFunc.callbackFunc())});
}

inline Promise Promise::onThenAndFinally(
    CallbackFuncWrapper &&onFulfilledFunc, CallbackFuncWrapper &&onRejectedFunc,
    CallbackFuncWrapper &&onFinallyFunc)
{
    return onThen(std::move(onFulfilledFunc), std::move(onRejectedFunc)).onFinally(std::move(onFinallyFunc));
}

inline std::vector<napi_value> unwrapValues(napi_env env, const std::vector<ValueWrapper> &wrappedValues)
{
    std::vector<napi_value> unwrappedValues;
    std::transform(
        wrappedValues.begin(), wrappedValues.end(),
        std::back_inserter(unwrappedValues),
        [&](const ValueWrapper &arg) {
            return arg.mapToValue(env);
        });
    return unwrappedValues;
}

inline Napi::Error makeLoggedException(napi_env env, const std::string &msg)
{
    return details_qnapi_p_h::makeLoggedExceptionImpl(env, msg);
}

template<typename T>
bool valueTypeMatches(const Napi::Value &value)
{
    return details_qnapi_p_h::valueTypeMatchesImpl<T>(value);
}

template<typename Element>
bool arrayElementTypesMatch(const Napi::Value &value)
{
    if (!valueTypeMatches<Napi::Array>(value)) {
        return false;
    }

    auto arrayValue = checkedCast<Napi::Array>(value);
    auto arrayLength = arrayValue.Length();

    bool allElementsMatch = true;
    for (std::size_t i = 0; i < arrayLength; ++i) {
        if (!valueTypeMatches<Element>(arrayValue.Get(i))) {
            allElementsMatch = false;
            break;
        }
    }

    return allElementsMatch;
}

inline std::string getValueTypeString(const Napi::Value &value)
{
    return details_qnapi_p_h::getValueTypeStringImpl(value);
}

template<typename T, typename ValueDescriptionSupplier>
T checkedCast(const Napi::Value &value, ValueDescriptionSupplier &&valueDescSupplier)
{
    return details_qnapi_p_h::checkedCastImpl<T>(
        value, std::forward<ValueDescriptionSupplier>(valueDescSupplier));
}

template<typename T>
T checkedCast(const Napi::Value &value)
{
    return details_qnapi_p_h::checkedCastImpl<T>(
        value,
        []() {
            return std::string();
        });
}

template<typename T>
T get(const Napi::Object &obj, const std::string &expr)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<T>(
        obj.Env(),
        [&]() {
            return details_qnapi_p_h::getWithContextImpl<T>(obj, expr).second;
        });
}

template<typename T, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, T>
get(const Napi::Reference<Obj> &objRef, const std::string &expr)
{
    using namespace std::string_literals;

    if (objRef.IsEmpty()) {
        throw makeLoggedException(
            objRef.Env(), "trying to evaluate expression using empty object reference: "s + expr);
    }

    return details_qnapi_p_h::runEscapingHandleScopeImpl<T>(
        objRef.Env(),
        [&]() {
            return get<T>(objRef.Value(), expr);
        });
}

template<typename OutputContainer, typename Element, typename TransFunc>
OutputContainer getArrayElements(const Napi::Array &inputArray, TransFunc &&transFunc)
{
    using namespace std::string_literals;

    OutputContainer result;
    auto arrayLength = inputArray.Length();
    for (std::size_t i = 0; i < arrayLength; ++i) {
        auto arg = inputArray.Get(i);
        if (!valueTypeMatches<Element>(arg)) {
            constexpr const char *expectedTypeName = details_qnapi_p_h::ValueTypeTraits<Element>::typeName;
            auto argTypeStr = getValueTypeString(arg);
            throw makeLoggedException(
                inputArray.Env(),
                "wrong type of Napi array element #"s + std::to_string(i)
                + ", expected '"s + expectedTypeName + "', got '"s + argTypeStr + "'"s);
        }
        result.insert(result.end(), transFunc(checkedCast<Element>(arg)));
    }

    return result;
}

template<typename OutputContainer, typename Element>
OutputContainer getArrayElements(const Napi::Array &inputArray)
{
    return getArrayElements<OutputContainer, Element>(
        inputArray,
        [](Element &&elem) {
            return std::forward<Element>(elem);
        });
}

template<typename Result>
Result callMethod(
    const Napi::Object &obj, const std::string &methodName, const std::vector<napi_value> &args)
{
    return details_qnapi_p_h::callMethodImpl<Result>(obj, methodName, args);
}

template<typename Result, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, Result>
callMethod(const Napi::Reference<Obj> &objRef, const std::string &methodName, const std::vector<napi_value> &args)
{
    using namespace details_qnapi_p_h;
    using namespace std::string_literals;

    if (objRef.IsEmpty()) {
        throw makeLoggedExceptionImpl(
            objRef.Env(), "callMethod: trying to call a method on empty object reference: "s + methodName);
    }

    return details_qnapi_p_h::runEscapingHandleScopeImpl<Result>(
        objRef.Env(),
        [&]() {
            return callMethod<Result>(objRef.Value(), methodName, args);
        });
}

template<typename Result>
Result callMethod(
    const Napi::Object &obj, const std::string &methodName, std::initializer_list<ValueWrapper> args)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Result>(
        obj.Env(),
        [&]() {
            return callMethod<Result>(obj, methodName, unwrapValues(obj.Env(), args));
        });
}

template<typename Result, typename Obj>
std::enable_if_t<std::is_base_of<Napi::Object, Obj>::value, Result>
callMethod(const Napi::Reference<Obj> &objRef, const std::string &methodName, std::initializer_list<ValueWrapper> args)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Result>(
        objRef.Env(),
        [&]() {
            return callMethod<Result>(objRef, methodName, unwrapValues(objRef.Env(), args));
        });
}

inline Promise callThenOnPromise(
    const Napi::Promise &promise,
    std::function<void(const Napi::CallbackInfo &)> &&onFulfilledFunc,
    std::function<void(const Napi::CallbackInfo &)> &&onRejectedFunc)
{
    auto env = promise.Env();
    return callMethod<Napi::Promise>(
        promise, "then",
        {
            onFulfilledFunc ? Napi::Function::New(env, std::move(onFulfilledFunc)) : env.Undefined(),
            onRejectedFunc ? Napi::Function::New(env, std::move(onRejectedFunc)) : env.Undefined(),
        });
}

inline Promise callThenAndFinallyOnPromise(
    const Napi::Promise &promise,
    std::function<void(const Napi::CallbackInfo &)> &&onFulfilledFunc,
    std::function<void(const Napi::CallbackInfo &)> &&onRejectedFunc,
    std::function<void(const Napi::CallbackInfo &)> &&finallyFunc)
{
    return callMethod<Napi::Promise>(
        callThenOnPromise(promise, std::move(onFulfilledFunc), std::move(onRejectedFunc)),
        "finally",
        {
            std::move(finallyFunc),
        });
}

inline Object makeNewInstance(const Napi::Function &type, const std::vector<ValueWrapper> &args)
{
    return type.New(unwrapValues(type.Env(), args));
}

inline Object makeNewInstance(const Napi::Object &baseObj, const std::string &typePath, const std::vector<ValueWrapper> &args)
{
    return makeNewInstance(get<Napi::Function>(baseObj, typePath), args);
}

template<typename... Args>
void getLeadingArgs(const std::string &funcName, const Napi::CallbackInfo &cbInfo, Args &...args)
{
    details_qnapi_p_h::getLeadingArgsImpl(
        funcName, cbInfo, std::tie(args...), std::make_index_sequence<sizeof...(Args)>());
}

template<typename Arg>
Arg getFirstArg(const std::string &funcName, const Napi::CallbackInfo &cbInfo)
{
    Arg arg;
    getLeadingArgs(funcName, cbInfo, arg);
    return arg;
}

template<typename T>
std::shared_ptr<Napi::Reference<T>> makeSharedPersistent(const T &value)
{
    return std::make_shared<Napi::Reference<T>>(Napi::Persistent(value));
}

inline Napi::Value getPropOrUndefined(const Napi::Value &obj, const std::string &propName)
{
    if (!obj.IsObject()) {
        return obj.Env().Undefined();
    }

    auto typedObj = checkedCast<Napi::Object>(obj);

    return typedObj.Has(propName)
        ? typedObj.Get(propName)
        : typedObj.Env().Undefined();
}

template<typename T>
Napi::Value getPropOrUndefined(const Napi::Reference<T> &objRef, const std::string &propName)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Napi::Value>(
        objRef.Env(),
        [&]() {
            return getPropOrUndefined(objRef.Value(), propName);
        });
}

template<typename T>
std::enable_if_t<std::is_base_of<Napi::Value, T>::value, T>
getOptionalPropOrEmpty(const Napi::Object &optObj, const std::string &propName, const std::string &objDesc)
{
    using namespace std::string_literals;

    if (optObj.IsEmpty() || !optObj.Has(propName)) {
        return T();
    }

    auto propValue = optObj.Get(propName);

    return !propValue.IsUndefined()
        ? QNapi::checkedCast<T>(
            propValue,
            [&]() {
                auto baseDesc = "property '"s + propName + "' of object"s;
                return !objDesc.empty()
                    ? baseDesc + ": "s + objDesc
                    : baseDesc;
            })
        : T();
}

inline Object makeObject(napi_env env, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    auto obj = Napi::Object::New(env);
    setProps(obj, namedValues);
    return obj;
}

inline Array makeArray(napi_env env, const std::vector<ValueWrapper> &values)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Napi::Array>(
        env,
        [&]() {
            auto array = Napi::Array::New(env, values.size());
            for (std::size_t i = 0; i < values.size(); ++i) {
                array.Set(i, values[i].mapToValue(env));
            }
            return array;
        });
}

inline void setProp(Napi::Object &obj, const std::string &name, const ValueWrapper &value)
{
    Napi::HandleScope setPropScope(obj.Env());
    obj.Set(name, value.mapToValue(obj.Env()));
}

inline void setProps(Napi::Object &obj, const std::vector<std::pair<std::string, ValueWrapper>> &namedValues)
{
    Napi::HandleScope setPropsScope(obj.Env());
    auto env = obj.Env();
    for (const auto &namedValue : namedValues) {
        obj.Set(namedValue.first, namedValue.second.mapToValue(env));
    }
}

template<typename Result, typename F>
Result runEscapingHandleScope(napi_env env, F &&func)
{
    return details_qnapi_p_h::runEscapingHandleScopeImpl<Result>(env, std::forward<F>(func));
}

template<typename Enum, std::size_t EnumeratorsCount>
std::array<std::pair<Enum, double>, EnumeratorsCount> resolveEnumeratorsValues(
    const Napi::Object &enumModule, const std::string &enumName,
    const std::pair<Enum, const char *> (&enumeratorsNames)[EnumeratorsCount])
{
    using namespace std::string_literals;

    Napi::Value enumObjValue = getPropOrUndefined(enumModule, enumName);
    if (!enumObjValue.IsObject()) {
        throw makeLoggedException(
            enumModule.Env(), "JS module doesn't contain the enum: "s + enumName);
    }

    std::array<std::pair<Enum, double>, EnumeratorsCount> enumeratorsValues;

    for (std::size_t i = 0; i < EnumeratorsCount; ++i) {
        const auto enumeratorName = enumeratorsNames[i].second;

        Napi::Value jsEnumeratorValue = getPropOrUndefined(enumObjValue, enumeratorName);
        if (!jsEnumeratorValue.IsNumber()) {
            throw makeLoggedException(
                enumModule.Env(), "JS enum '"s + enumName + "' doesn't contain: "s + enumeratorName);
        }

        enumeratorsValues[i] = std::make_pair(
            enumeratorsNames[i].first,
            checkedCast<Napi::Number>(jsEnumeratorValue).DoubleValue());
    }

    return enumeratorsValues;
}

template<typename Enum, std::size_t EnumeratorsCount>
std::function<Enum(Napi::Number)> makeFromJsEnumMapper(
    const Napi::Object &enumModule, const std::string &enumName,
    const std::pair<Enum, const char *> (&enumeratorsNames)[EnumeratorsCount])
{
    using namespace std::string_literals;

    auto enumeratorsValues = resolveEnumeratorsValues(enumModule, enumName, enumeratorsNames);

    return [enumName, enumeratorsValues](Napi::Number enumeratorValue) {
        double enumeratorDoubleValue = enumeratorValue.DoubleValue();

        auto valueIter = std::find_if(
            enumeratorsValues.begin(), enumeratorsValues.end(),
            [&](const auto &valuePair) {
                return valuePair.second == enumeratorDoubleValue;
            });

        if (valueIter == enumeratorsValues.end()) {
            throw makeLoggedException(
                enumeratorValue.Env(),
                "Illegal Napi value of enumerator for enum '"s + enumName + "': "
                + std::to_string(enumeratorDoubleValue));
        }

        return valueIter->first;
    };
}

}

QT_END_NAMESPACE

#endif // QNAPI_P_H

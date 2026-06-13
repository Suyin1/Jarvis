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

#ifndef QOHOSJSENV_H
#define QOHOSJSENV_H

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

#include <QtCore/private/qnapi_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <QtCore/qjsonarray.h>
#include <QtCore/qjsonobject.h>
#include <QtCore/qjsonvalue.h>
#include <QtCore/qstring.h>
#include <QtCore/qstringlist.h>

#include <algorithm>
#include <array>
#include <cstdint>
#include <iterator>
#include <limits>
#include <type_traits>
#include <memory>
#include <napi/native_api.h>
#include <napi.h>
#include <utility>

QT_BEGIN_NAMESPACE

struct Q_CORE_EXPORT QOhosJsEnv
{
    template <typename T, typename Enable = void>
    struct QNapiValue {
        static QNapi::Value create(napi_env /*env*/, T /*value*/) {
            static_assert(sizeof(T) == 0, "Unsupported type - provide proper QOhosJsEnv::QNapiValue"
                                          " overload");
        }
        static Napi::Maybe<T> get(const QNapi::Value &/*value*/) {
            static_assert(sizeof(T) == 0, "Unsupported type - provide proper QOhosJsEnv::QNapiValue"
                                          " overload");
        }
    };

    template <typename T>
    static QNapi::Value toNapiValue(napi_env env, T &&value) {
        return QNapiValue<typename std::decay<T>::type>::create(env, std::forward<T>(value));
    }

    template<typename ReturnType>
    static ReturnType fromNapiValue(const QNapi::Value &value)
    {
        return QNapiValue<typename std::decay<ReturnType>::type>::get(value).UnwrapOr(ReturnType());
    }
};

//
// overloads of QNapiValue to support various types
//

template<class T>
struct IsNapiNumberConvertible
    : std::integral_constant<
        bool,
        std::is_same<T, double>::value
        || (
            std::is_integral<T>::value
            && (
                (sizeof(T) <= sizeof(uint32_t))
                || (std::is_signed<T>::value && sizeof(T) <= sizeof(int64_t))))>
{
};

template <>
struct QOhosJsEnv::QNapiValue<napi_value> {
    static QNapi::Value create(napi_env, napi_value value);
    static Napi::Maybe<napi_value> get(const QNapi::Value &value);
};

template <>
struct QOhosJsEnv::QNapiValue<QString> {
    static QNapi::Value create(napi_env env, QString value);
    static Napi::Maybe<QString> get(const QNapi::Value &value);
};

template <typename T>
struct QOhosJsEnv::QNapiValue<T, std::enable_if_t<IsNapiNumberConvertible<T>::value>>
{
    static QNapi::Value create(napi_env env, T value);
    static Napi::Maybe<T> get(const QNapi::Value &value);
};

template <>
struct QOhosJsEnv::QNapiValue<bool> {
    static QNapi::Value create(napi_env env, bool value);
    static Napi::Maybe<bool> get(const QNapi::Value &value);
};

template<typename T>
struct QOhosJsEnv::QNapiValue<QList<T>>
{
    static QNapi::Value create(napi_env env, const QList<T> &inputValue);
    static Napi::Maybe<QList<T>> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QStringList>
{
    static QNapi::Value create(napi_env env, const QStringList &inputValue);
    static Napi::Maybe<QStringList> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonValue>
{
    static QNapi::Value create(napi_env env, const QJsonValue &inputValue);
    static Napi::Maybe<QJsonValue> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonArray>
{
    static QNapi::Value create(napi_env env, const QJsonArray &inputValue);
    static Napi::Maybe<QJsonArray> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QJsonObject>
{
    static QNapi::Value create(napi_env env, const QJsonObject &inputValue);
    static Napi::Maybe<QJsonObject> get(const QNapi::Value &inputValue);
};

template<>
struct QOhosJsEnv::QNapiValue<QByteArray>
{
    static QNapi::Value create(napi_env env, const QByteArray &inputValue);
    static Napi::Maybe<QByteArray> get(const QNapi::Value &inputValue);
};

inline QNapi::Value QOhosJsEnv::QNapiValue<napi_value>::create(napi_env env, napi_value value)
{
    return QNapi::Value(env, value);
}

inline Napi::Maybe<napi_value> QOhosJsEnv::QNapiValue<napi_value>::get(const QNapi::Value &value)
{
    return Napi::Just<napi_value>(value);
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QString>::create(napi_env env, QString value)
{
    return QNapi::String::New(env, value.toUtf8());
}

inline Napi::Maybe<QString> QOhosJsEnv::QNapiValue<QString>::get(const QNapi::Value &value)
{
    return value.IsString()
        ? Napi::Just(QString::fromStdString(QNapi::checkedCast<QNapi::String>(value).Utf8Value()))
        : Napi::Nothing<QString>();
}

template <typename T>
QNapi::Value QOhosJsEnv::QNapiValue<T, std::enable_if_t<IsNapiNumberConvertible<T>::value>>::create(napi_env env, T value)
{
    return QNapi::Number::New(env, value);
}

template <typename T>
Napi::Maybe<T> QOhosJsEnv::QNapiValue<T, std::enable_if_t<IsNapiNumberConvertible<T>::value>>::get(const QNapi::Value &value)
{
    return value.IsNumber()
        ? Napi::Just<T>(QNapi::checkedCast<QNapi::Number>(value))
        : Napi::Nothing<T>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<bool>::create(napi_env env, bool value)
{
    return QNapi::Boolean::New(env, value);
}

inline Napi::Maybe<bool> QOhosJsEnv::QNapiValue<bool>::get(const QNapi::Value &value)
{
    return value.IsBoolean()
        ? Napi::Just<bool>(QNapi::checkedCast<QNapi::Boolean>(value))
        : Napi::Nothing<bool>();
}

template<typename T>
QNapi::Value QOhosJsEnv::QNapiValue<QList<T>>::create(napi_env env, const QList<T> &inputValue)
{
    return QNapi::runEscapingHandleScope<QNapi::Array>(
        env,
        [&]() {
            auto outputArray = QNapi::Array::New(env, inputValue.length());
            for (int i = 0; i < inputValue.length(); ++i) {
                outputArray.Set(i, QNapiValue<T>::create(env, inputValue[i]));
            }
            return outputArray;
        });
}

template<typename T>
Napi::Maybe<QList<T>> QOhosJsEnv::QNapiValue<QList<T>>::get(const QNapi::Value &inputValue)
{
    if (!inputValue.IsArray()) {
        return Napi::Nothing<QList<T>>();
    }

    auto inputArray = QNapi::checkedCast<QNapi::Array>(inputValue);
    std::uint32_t rawInputArrayLength = inputArray.Length();
    if (rawInputArrayLength > std::numeric_limits<int>::max()) {
        return Napi::Nothing<QList<T>>();
    }
    int inputArrayLength = static_cast<int>(rawInputArrayLength);

    QList<T> outputList;
    outputList.reserve(inputArrayLength);

    for (int i = 0; i < inputArrayLength; ++i) {
        Napi::HandleScope inputElementScope{inputValue.Env()};
        auto optOutputElem = QNapiValue<T>::get(inputArray.Get(i));
        if (optOutputElem.IsNothing()) {
            break;
        }
        outputList.append(optOutputElem.Unwrap());
    }

    return outputList.length() == inputArrayLength
        ? Napi::Just(outputList)
        : Napi::Nothing<QList<T>>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QStringList>::create(napi_env env, const QStringList &inputValue)
{
    return QNapiValue<QList<QString>>::create(env, inputValue);
}

inline Napi::Maybe<QStringList> QOhosJsEnv::QNapiValue<QStringList>::get(const QNapi::Value &inputValue)
{
    const auto listValue = QNapiValue<QList<QString>>::get(inputValue);
    return listValue.IsJust() ? Napi::Just<QStringList>(listValue.Unwrap()) : Napi::Nothing<QStringList>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonValue>::create(napi_env env, const QJsonValue &inputValue)
{
    switch (inputValue.type()) {
    case QJsonValue::Type::Array:
        return QNapiValue<QJsonArray>::create(env, inputValue.toArray());
    case QJsonValue::Type::Bool:
        return QNapiValue<bool>::create(env, inputValue.toBool());
    case QJsonValue::Type::Double:
        return QNapiValue<double>::create(env, inputValue.toDouble());
    case QJsonValue::Type::Null:
    case QJsonValue::Type::Undefined:
        return Napi::Env(env).Null();
    case QJsonValue::Type::Object:
        return QNapiValue<QJsonObject>::create(env, inputValue.toObject());
    case QJsonValue::Type::String:
        return QNapiValue<QString>::create(env, inputValue.toString());
    }

    throw Napi::Error::New(env, "Got unsupported (impossible) QJsonValue");
}

inline Napi::Maybe<QJsonValue> QOhosJsEnv::QNapiValue<QJsonValue>::get(const QNapi::Value &inputValue)
{
    if (inputValue.IsArray()) {
        const auto arrayValue = QNapiValue<QJsonArray>::get(inputValue);
        return arrayValue.IsJust() ? Napi::Just<QJsonValue>(arrayValue.Unwrap()) : Napi::Nothing<QJsonValue>();
    } else if (inputValue.IsBoolean()) {
        const auto boolValue = QNapi::checkedCast<QNapi::Boolean>(inputValue).Value();
        return Napi::Just(QJsonValue(boolValue));
    } else if (inputValue.IsNumber()) {
        const auto numberValue = QNapi::checkedCast<QNapi::Number>(inputValue).DoubleValue();
        return Napi::Just(QJsonValue(numberValue));
    } else if (inputValue.IsObject()) {
        const auto objectValue = QNapiValue<QJsonObject>::get(inputValue);
        return objectValue.IsJust() ? Napi::Just<QJsonValue>(objectValue.Unwrap()) : Napi::Nothing<QJsonValue>();
    } else if (inputValue.IsString()) {
        const auto stringValue = QNapiValue<QString>::get(inputValue);
        return stringValue.IsJust() ? Napi::Just<QJsonValue>(stringValue.Unwrap()) : Napi::Nothing<QJsonValue>();
    } else {
        return Napi::Nothing<QJsonValue>();
    }
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonArray>::create(napi_env env, const QJsonArray &inputValue)
{
    QList<QJsonValue> listInputValue;
    listInputValue.reserve(inputValue.size());
    std::copy(inputValue.begin(), inputValue.end(), std::back_inserter(listInputValue));
    return QNapiValue<QList<QJsonValue>>::create(env, listInputValue);
}

inline Napi::Maybe<QJsonArray> QOhosJsEnv::QNapiValue<QJsonArray>::get(const QNapi::Value &inputValue)
{
    const auto transform = [](const QList<QJsonValue> &input) -> QJsonArray {
        QJsonArray result;
        std::copy(input.begin(), input.end(), std::back_inserter(result));
        return result;
    };

    const auto listValue = QNapiValue<QList<QJsonValue>>::get(inputValue);
    return listValue.IsJust()
        ? Napi::Just(transform(listValue.Unwrap()))
        : Napi::Nothing<QJsonArray>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QJsonObject>::create(napi_env env, const QJsonObject &inputValue)
{
    return QNapi::runEscapingHandleScope<QNapi::Object>(
        env,
        [&]() {
            auto outputObject = QNapi::Object::New(env);
            for (auto inputValueIter = inputValue.begin(); inputValueIter != inputValue.end(); ++inputValueIter) {
                outputObject.Set(
                    inputValueIter.key().toStdString(),
                    QNapiValue<QJsonValue>::create(env, inputValueIter.value()));
            }
            return outputObject;
        });
}

inline Napi::Maybe<QJsonObject> QOhosJsEnv::QNapiValue<QJsonObject>::get(const QNapi::Value &inputValue)
{
    if (!inputValue.IsObject()) {
        return Napi::Nothing<QJsonObject>();
    }

    auto inputObject = QNapi::checkedCast<QNapi::Object>(inputValue);

    QJsonObject result;
    bool allElementsSet = true;

    for (const auto &inputElement : inputObject) {
        if (inputElement.first.IsString()) {
            const auto key = QNapiValue<QString>::get(inputElement.first).Unwrap();

            auto optPropValue = QNapiValue<QJsonValue>::get(inputElement.second);
            if (optPropValue.IsNothing()) {
                allElementsSet = false;
                break;
            }

            result[key] = optPropValue.Unwrap();
        }
    }

    return allElementsSet
        ? Napi::Just(result)
        : Napi::Nothing<QJsonObject>();
}

inline QNapi::Value QOhosJsEnv::QNapiValue<QByteArray>::create(napi_env env, const QByteArray &inputValue)
{
    auto arrayBuffer = QNapi::ArrayBuffer::New(env, inputValue.length());
    std::memcpy(arrayBuffer.Data(), inputValue.data(), inputValue.length());
    return arrayBuffer;
}

inline Napi::Maybe<QByteArray> QOhosJsEnv::QNapiValue<QByteArray>::get(const QNapi::Value &inputValue)
{
    if (!inputValue.IsArrayBuffer()) {
        return Napi::Nothing<QByteArray>();
    }

    auto buff = QNapi::checkedCast<QNapi::ArrayBuffer>(inputValue);

    return Napi::Just(QByteArray(static_cast<char *>(buff.Data()), buff.ByteLength()));
}

QT_END_NAMESPACE

#endif // QOHOSJSENV_H

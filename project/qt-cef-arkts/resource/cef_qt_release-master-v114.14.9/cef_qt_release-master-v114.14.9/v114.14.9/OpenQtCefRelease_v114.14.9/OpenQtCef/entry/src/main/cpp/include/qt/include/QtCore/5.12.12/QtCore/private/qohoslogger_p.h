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
#ifndef QOHOSLOGGER_H
#define QOHOSLOGGER_H

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

#include <QtCore/qdebug.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qthread.h>
#include <cstdarg>
#include <cstdio>
#include <hilog/log.h>

QT_BEGIN_NAMESPACE

Q_CORE_EXPORT const QLoggingCategory &QtForOhos();

#define qOhosDebug(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCDebug(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosWarning(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCWarning(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosCritical(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCCritical(category) \
    _Pragma("GCC diagnostic pop")

#define qOhosFatal(category) \
    _Pragma("GCC diagnostic push") \
    _Pragma("GCC diagnostic ignored \"-Wgnu-zero-variadic-macro-arguments\"") \
    qCFatal(category) \
    _Pragma("GCC diagnostic pop")

Q_ATTRIBUTE_FORMAT_PRINTF(2, 0) inline void qOhosVPrintf(LogLevel logLevel, const char *format, std::va_list ap)
{
    std::array<char, 256> msgBuffer;
    if (std::vsnprintf(msgBuffer.data(), msgBuffer.size(), format, ap) < 0) {
        std::snprintf(msgBuffer.data(), msgBuffer.size(), "[error formatting log msg: %s]", format);
    }

    OH_LOG_Print(
        LOG_APP, logLevel, LOG_DOMAIN, "OHOS plugin",
        "[QtForOhos]: T: 0x%{public}llx, M: %{public}s",
        reinterpret_cast<unsigned long long>(QThread::currentThreadId()), msgBuffer.data());
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfDebug(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_DEBUG, format, ap);
    va_end(ap);
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfWarning(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_WARN, format, ap);
    va_end(ap);
}

Q_ATTRIBUTE_FORMAT_PRINTF(1, 2) inline void qOhosPrintfError(const char *format, ...)
{
    std::va_list ap;
    va_start(ap, format);
    qOhosVPrintf(LOG_ERROR, format, ap);
    va_end(ap);
}

template<typename StringType>
struct QCScopedDebug
{
    QCScopedDebug(StringType message):
    m_message(message)
    { qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << ", M:" << message << "begin";}

    ~QCScopedDebug() { qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << ", M:" << m_message
                                      << "end";}
private:
    StringType m_message;
};

template<typename StringType>
struct QCScopedDebugJS
{
    QCScopedDebugJS(StringType message): m_message(message) {qOhosPrintfDebug("%s begin", message);}
    ~QCScopedDebugJS() {qOhosPrintfDebug("%s end", m_message);}

private:
    StringType m_message;
};

template<typename StringType>
auto make_QCScopedDebug(StringType&& message) -> QCScopedDebug<typename std::decay<StringType>::type> {
    return {std::forward<StringType>(message)};
}

template<typename StringType>
auto make_QCScopedDebugJS(StringType&& message) -> QCScopedDebugJS<typename std::decay<StringType>::type> {
    return {std::forward<StringType>(message)};
}

#define DUMP(x) qOhosDebug(QtForOhos) << "T:" << QThread::currentThreadId() << #x << x;

QT_END_NAMESPACE

#endif // QOHOSLOGGER_H

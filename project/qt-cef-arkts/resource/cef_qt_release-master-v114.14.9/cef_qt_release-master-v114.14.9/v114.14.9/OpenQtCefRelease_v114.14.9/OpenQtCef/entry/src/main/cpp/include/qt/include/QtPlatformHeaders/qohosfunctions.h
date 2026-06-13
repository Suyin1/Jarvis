/****************************************************************************
**
** Copyright (C) 2024 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#ifndef QOHOSWINDOWFUNCTIONS_H
#define QOHOSWINDOWFUNCTIONS_H

#include <QtCore/QByteArray>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

class QWindow;

class QOhosFunctions
{
public:
    typedef void (*TagWindowAsSubWindowOf)(QObject *, QWindow *);
    typedef void (*TagWindowOrWidgetAsMainWindow)(QObject *);
    typedef QWindow *(*GetWindowOrWidgetAsSubWindowOfTagValue)(QObject *);
    static const QByteArray tagWindowOrWidgetAsSubwindowOfIdentifier() { return QByteArrayLiteral("tagWindowOrWidgetAsSubWindowOf"); }
    static const QByteArray tagWindowOrWidgetAsMainWindowIdentifier() { return QByteArrayLiteral("tagWindowOrWidgetAsMainWindowIdentifier"); }
    static const QByteArray getWindowOrWidgetAsSubWindowOfTagValueIdentifier() { return QByteArrayLiteral("tagWindowOrWidgetAsSubWindowOf"); }

    static void tagWindowOrWidgetAsSubWindowOf(QObject *windowOrWidgetToTag, QWindow *mainWindow)
    {
        static auto func = reinterpret_cast<TagWindowAsSubWindowOf>(
            QGuiApplication::platformFunction(tagWindowOrWidgetAsSubwindowOfIdentifier()));
        Q_ASSERT(func);
        func(windowOrWidgetToTag, mainWindow);
    }

    static void tagWindowOrWidgetAsMainWindow(QObject *windowOrWidgetToTag)
    {
        static auto func = reinterpret_cast<TagWindowOrWidgetAsMainWindow>(
            QGuiApplication::platformFunction(tagWindowOrWidgetAsSubwindowOfIdentifier()));
        Q_ASSERT(func);
        func(windowOrWidgetToTag);
    }

    static QWindow *getWindowOrWidgetAsSubWindowOfTagValue(QObject *targetWindowOrWidget)
    {
        static auto func = reinterpret_cast<GetWindowOrWidgetAsSubWindowOfTagValue>(
            QGuiApplication::platformFunction(tagWindowOrWidgetAsSubwindowOfIdentifier()));
        Q_ASSERT(func);
        return func(targetWindowOrWidget);
    }
};

QT_END_NAMESPACE

#endif // QOHOSWINDOWFUNCTIONS_H


/****************************************************************************
**
** Copyright (C) 2023 The Qt Company Ltd.
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

#ifndef QOHOSUIABILITYCONTEXT_H
#define QOHOSUIABILITYCONTEXT_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qstring.h>
#include <QtOhosExtras/qohosoperationstatus.h>
#include <QtOhosExtras/qohoswant.h>
#include <QtOhosExtras/qohosstartoptions.h>
#include <QtOhosExtras/qtohosextrasglobal.h>
#include <QtGui/QWindow>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

namespace QtOhosExtras
{

class Q_OHOS_EXTRAS_EXPORT QOhosUiAbilityContext : public QObject
{
    Q_OBJECT

public:
    static QOhosUiAbilityContext *instance();

Q_SIGNALS:
    void newWantReceived(QOhosWant want);

private:
    QOhosUiAbilityContext();

    Q_DISABLE_COPY(QOhosUiAbilityContext)
};

Q_OHOS_EXTRAS_EXPORT QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want);
Q_OHOS_EXTRAS_EXPORT QSharedPointer<QOhosOperationStatus> startAbility(const QOhosWant &want, const QOhosStartOptions &options);

Q_OHOS_EXTRAS_EXPORT void startNewAbilityInstance(QWidget *instanceWidget);

Q_OHOS_EXTRAS_EXPORT void startAppProcess(const QString &processId, const QOhosWant &requestWant);
Q_OHOS_EXTRAS_EXPORT void startAppProcess(const QString &processId, const QOhosWant &requestWant, const QOhosStartOptions &options);

Q_OHOS_EXTRAS_EXPORT void setAbilityInstanceDestroyEnabled(QWindow *instanceWindow, bool destroyEnabled);

}

QT_END_NAMESPACE

#endif // QOHOSUIABILITYCONTEXT_H

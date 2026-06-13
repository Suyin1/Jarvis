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

#ifndef QOHOSSTARTOPTIONS_H
#define QOHOSSTARTOPTIONS_H

#include <QtCore/qsharedpointer.h>
#include <QtOhosExtras/qtohosextrasglobal.h>

QT_BEGIN_NAMESPACE

namespace QtOhosExtras
{

class QOhosStartOptions
{
public:
    enum class ProcessMode
    {
        NEW_PROCESS_ATTACH_TO_PARENT,
        NEW_PROCESS_ATTACH_TO_STATUS_BAR_ITEM,
    };

    enum class StartupVisibility
    {
        STARTUP_HIDE,
        STARTUP_SHOW,
    };

    enum class WindowMode
    {
        WINDOW_MODE_SPLIT_PRIMARY,
        WINDOW_MODE_SPLIT_SECONDARY,
    };

    virtual ~QOhosStartOptions();

    virtual void setWindowMode(WindowMode windowMode) = 0;
    virtual void setDisplayId(int displayId) = 0;
    virtual void setWithAnimation(bool withAnimation) = 0;
    virtual void setWindowLeft(int windowLeft) = 0;
    virtual void setWindowTop(int windowTop) = 0;
    virtual void setWindowWidth(int windowWidth) = 0;
    virtual void setWindowHeight(int windowHeight) = 0;
    virtual void setProcessMode(ProcessMode processMode) = 0;
    virtual void setStartupVisibility(StartupVisibility startupVisiblity) = 0;
    virtual void setWindowFocused(bool windowFocused) = 0;

protected:
    QOhosStartOptions();

private:
    Q_DISABLE_COPY(QOhosStartOptions)
};

Q_OHOS_EXTRAS_EXPORT QSharedPointer<QOhosStartOptions> createStartOptions();

}

QT_END_NAMESPACE

#endif

/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtSystems module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "ipcendpoint_p.h"

#include <QEventLoop>
#include <QTimer>

QT_BEGIN_NAMESPACE
/*!
    QServiceIpcEndPoint
*/
QServiceIpcEndPoint::QServiceIpcEndPoint(QObject* parent)
    : QObject( parent )
{
}

QServiceIpcEndPoint::~QServiceIpcEndPoint()
{
    incoming.clear();
}

bool QServiceIpcEndPoint::packageAvailable() const
{
    return !incoming.isEmpty();
}

QServicePackage QServiceIpcEndPoint::nextPackage()
{
    if (!incoming.isEmpty())
        return incoming.dequeue();
    return QServicePackage();
}

void QServiceIpcEndPoint::writePackage(QServicePackage newPackage)
{
    flushPackage(newPackage);
}

void QServiceIpcEndPoint::getSecurityCredentials(QServiceClientCredentials &)
{
}

void QServiceIpcEndPoint::terminateConnection()
{
    qWarning() << "SFW Terminate connection called on base class, should be reimplemented to do something";
}

int QServiceIpcEndPoint::waitForData()
{
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    connect(&timer, SIGNAL(timeout()), &loop, SLOT(quit()));
    connect(this, SIGNAL(packageReceived()), &loop, SLOT(quit()));

    timer.start(30000);
    loop.exec();
    return 0;
}

void QServiceIpcEndPoint::waitingDone()
{
    emit packageReceived();
}

#include "moc_ipcendpoint_p.cpp"
QT_END_NAMESPACE

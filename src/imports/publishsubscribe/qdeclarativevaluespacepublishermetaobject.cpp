/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSystems module of the Qt Toolkit.
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

#include "qdeclarativevaluespacepublishermetaobject_p.h"

QT_BEGIN_NAMESPACE

QDeclarativeValueSpacePublisherMetaObject::QDeclarativeValueSpacePublisherMetaObject(QDeclarativeValueSpacePublisher *parent)
    : metaObject(0)
    , object(parent)
{
    metaObjectBuilder.setSuperClass(parent->metaObject());
    metaObjectBuilder.setClassName(parent->metaObject()->className());
    metaObjectBuilder.setFlags(QMetaObjectBuilder::DynamicMetaObject);
    metaObject = metaObjectBuilder.toMetaObject();
    signalOffset = metaObject->methodOffset();
    propertyOffset = metaObject->propertyCount();
    d = metaObject->d;
}

QDeclarativeValueSpacePublisherMetaObject::~QDeclarativeValueSpacePublisherMetaObject()
{
    ::free(metaObject);
}

int QDeclarativeValueSpacePublisherMetaObject::createProperty(const char *name, const char *type)
{
    int id = metaObjectBuilder.propertyCount();
    metaObjectBuilder.addSignal("__" + QByteArray::number(id) + "()");
    metaObjectBuilder.addProperty(name, type, id);

    ::free(metaObject);
    metaObject = metaObjectBuilder.toMetaObject();
    d = metaObject->d;

    dynamicProperties.insert(id, QPair<QString, QVariant>(QString::fromUtf8(name), QVariant()));

    return propertyOffset + id;
}

int QDeclarativeValueSpacePublisherMetaObject::metaCall(QMetaObject::Call c, int id, void **a)
{
    if ((c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty)
        && id >= propertyOffset) {
        if (c == QMetaObject::ReadProperty) {
            *reinterpret_cast<QVariant*>(a[0]) = dynamicProperties[id - propertyOffset].second;
        } else if (c == QMetaObject::WriteProperty) {
            dynamicProperties[id - propertyOffset].second = *reinterpret_cast<QVariant*>(a[0]);
            activate(object, signalOffset + id, 0);
        }
        return -1;
    } else {
        return object->qt_metacall(c, id, a);
    }
}

QT_END_NAMESPACE

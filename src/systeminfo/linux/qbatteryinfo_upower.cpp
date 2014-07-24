/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtSystems module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qbatteryinfo_upower_p.h"

#include <QtCore/qdir.h>
#include <QtCore/qfile.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qtimer.h>
#include <QtCore/qnumeric.h>
#include <QFile>
#include "qdevicekitservice_linux_p.h"


QT_BEGIN_NAMESPACE

QBatteryInfoPrivate::QBatteryInfoPrivate(QBatteryInfo *parent)
    : QObject(parent),
      cType(QBatteryInfo::UnknownCharger),
      cState(QBatteryInfo::UnknownChargingState),
      q_ptr(parent),
      index(0)
{
    initialize();
}

QBatteryInfoPrivate::QBatteryInfoPrivate(int batteryIndex, QBatteryInfo *parent)
    : QObject(parent),
      cType(QBatteryInfo::UnknownCharger),
      cState(QBatteryInfo::UnknownChargingState),
      q_ptr(parent),
      index(batteryIndex)
{
    initialize();
}

void QBatteryInfoPrivate::initialize()
{
    watcher = new QDBusServiceWatcher(QStringLiteral("org.freedesktop.UPower"),QDBusConnection::systemBus(),
                                      QDBusServiceWatcher::WatchForRegistration |
                                      QDBusServiceWatcher::WatchForUnregistration, this);
    connect(watcher, SIGNAL(serviceRegistered(QString)),
            this, SLOT(connectToUpower()));
    connect(watcher, SIGNAL(serviceUnregistered(QString)),
            this, SLOT(disconnectFromUpower()));

    bool uPowerAvailable = QDBusConnection::systemBus().interface()->isServiceRegistered(QStringLiteral("org.freedesktop.UPower"));

    if (uPowerAvailable)
        connectToUpower();
}

QBatteryInfoPrivate::~QBatteryInfoPrivate()
{
}

void QBatteryInfoPrivate::connectToUpower()
{
    getBatteryStats();
}

void QBatteryInfoPrivate::disconnectFromUpower()
{
}

int QBatteryInfoPrivate::batteryCount()
{
    return batteryMap.count();
}

int QBatteryInfoPrivate::batteryIndex() const
{
    return index;
}

bool QBatteryInfoPrivate::isValid()
{
    // valid if the index < total count.
    return (index >= 0) && (index < batteryCount());
}

void QBatteryInfoPrivate::setBatteryIndex(int batteryIndex)
{
    if (index != batteryIndex) {
        bool validBefore = isValid();
        int oldIndex = index;
        index = batteryIndex;
        bool validNow = isValid();
        if (validBefore != validNow)
            Q_EMIT validChanged(validNow);

        if (validNow) {
            if (validBefore) {
                // valid now, valid before so we have to check individual values

                // ignore chargerType - it won't change based on battery index
                //emit chargerTypeChanged(newChargerType);

                QBatteryInfo::ChargingState newChargingState = chargingState();
                if (newChargingState != chargingState(oldIndex))
                    emit chargingStateChanged(newChargingState);

                int newValue = level();
                if (newValue != level(oldIndex))
                    emit levelChanged(newValue);

                newValue = currentFlow();
                if (newValue != currentFlow(oldIndex))
                    emit currentFlowChanged(newValue);

                newValue = cycleCount();
                if (newValue != cycleCount(oldIndex))
                    emit cycleCountChanged(newValue);

                newValue = remainingCapacity();
                if (newValue != remainingCapacity(oldIndex))
                    emit remainingCapacityChanged(newValue);

                newValue = remainingChargingTime();
                if (newValue != remainingChargingTime(oldIndex))
                    emit remainingChargingTimeChanged(newValue);

                newValue = voltage();
                if (newValue != voltage(oldIndex))
                    emit voltageChanged(newValue);

                QBatteryInfo::LevelStatus newLevelStatus = levelStatus();
                if (newLevelStatus != levelStatus(oldIndex))
                    emit levelStatusChanged(newLevelStatus);

                QBatteryInfo::Health newHealth = health();
                if (newHealth != health(oldIndex))
                    emit healthChanged(newHealth);

                float newTemperature = temperature();
                if (!qFuzzyCompare(newTemperature, temperature(oldIndex)))
                    emit temperatureChanged(newTemperature);
            } else {
                // it wasn't valid before so everything is changed

                // ignore chargerType - it won't change based on battery index
                //emit chargerTypeChanged(newChargerType);

                emit chargingStateChanged(chargingState());
                emit levelChanged(level());
                emit currentFlowChanged(currentFlow());
                emit cycleCountChanged(cycleCount());
                emit remainingCapacityChanged(remainingCapacity());
                emit remainingChargingTimeChanged(remainingChargingTime());
                emit voltageChanged(voltage());
                emit levelStatusChanged(levelStatus());
                emit healthChanged(health());
                emit temperatureChanged(temperature());
            }
        }

        emit batteryIndexChanged(index);
    }
}

int QBatteryInfoPrivate::level(int battery)
{
    int maxCapacity = maximumCapacity(battery);
    int remCapacity = remainingCapacity(battery);

    if (maxCapacity == 0)
        return -1;

    return remCapacity * 100 / maxCapacity;
}

int QBatteryInfoPrivate::level()
{
    return level(index);
}

int QBatteryInfoPrivate::currentFlow(int battery)
{
    if (batteryMap.count() >= battery)
        return (batteryMap.value(battery).value(QStringLiteral("EnergyRate")).toDouble()
                / (batteryMap.value(battery).value(QStringLiteral("Voltage")).toDouble()) * 1000);
    else
        return 0;
}

int QBatteryInfoPrivate::currentFlow()
{
    return currentFlow(index);
}

int QBatteryInfoPrivate::cycleCount(int battery)
{
    Q_UNUSED(battery)

    return -1;
}

int QBatteryInfoPrivate::cycleCount()
{
    return cycleCount(index);
}

int QBatteryInfoPrivate::maximumCapacity(int battery)
{
    if (batteryMap.count() >= battery)
        return batteryMap.value(battery).value(QStringLiteral("EnergyFull")).toDouble() * 1000;
    else
        return 0;
}

int QBatteryInfoPrivate::maximumCapacity()
{
    return maximumCapacity(index);
}

int QBatteryInfoPrivate::remainingCapacity(int battery)
{
    if (batteryMap.count() >= battery)
        return batteryMap.value(battery).value(QStringLiteral("Energy")).toDouble() * 1000;
    else
        return 0;
}

int QBatteryInfoPrivate::remainingCapacity()
{
    return remainingCapacity(index);
}

int QBatteryInfoPrivate::remainingChargingTime(int battery)
{
    if (batteryMap.count() >= battery)
        return batteryMap.value(battery).value(QStringLiteral("TimeToFull")).toInt();
    else
        return 0;
}

int QBatteryInfoPrivate::remainingChargingTime()
{
    return remainingChargingTime(index);
}

int QBatteryInfoPrivate::voltage(int battery)
{
    if (batteryMap.count() >= battery)
        return (batteryMap.value(battery).value(QStringLiteral("Voltage")).toDouble() * 1000);
    else
        return 0;
}

int QBatteryInfoPrivate::voltage()
{
    return voltage(index);
}

QBatteryInfo::ChargerType QBatteryInfoPrivate::chargerType()
{
    return cType;
}

QBatteryInfo::ChargingState QBatteryInfoPrivate::chargingState(int battery)
{
    Q_UNUSED(battery)
    return cState;
}

QBatteryInfo::ChargingState QBatteryInfoPrivate::chargingState()
{
    return chargingState(index);
}

QBatteryInfo::LevelStatus QBatteryInfoPrivate::levelStatus(int battery)
{
    QBatteryInfo::LevelStatus stat = QBatteryInfo::LevelUnknown;
    if (batteryMap.count() >= battery) {
        int level = batteryMap.value(battery).value(QStringLiteral("Percentage")).toInt();
        if (level < 3)
            stat = QBatteryInfo::LevelEmpty;
        else if (level < 11)
            stat = QBatteryInfo::LevelLow;
        else if (level < 99)
            stat = QBatteryInfo::LevelOk;
        else
            stat = QBatteryInfo::LevelFull;
    }
    return stat;
}

QBatteryInfo::LevelStatus QBatteryInfoPrivate::levelStatus()
{
    return levelStatus(index);
}

QBatteryInfo::Health QBatteryInfoPrivate::health(int battery)
{
    Q_UNUSED(battery)

    return QBatteryInfo::HealthUnknown;
}

QBatteryInfo::Health QBatteryInfoPrivate::health()
{
    return health(index);
}

float QBatteryInfoPrivate::temperature(int battery)
{
    Q_UNUSED(battery)

    return qQNaN();
}

float QBatteryInfoPrivate::temperature()
{
    return temperature(index);
}

void QBatteryInfoPrivate::upowerDeviceChanged()
{
    QUPowerDeviceInterface *uPowerDevice = qobject_cast<QUPowerDeviceInterface*>(sender());

    if (uPowerDevice->type() == 1) {
//line power
        if (uPowerDevice->nativePath().contains(QStringLiteral("usb")))
            Q_EMIT chargerTypeChanged(QBatteryInfo::USBCharger);
        else
            Q_EMIT chargerTypeChanged(QBatteryInfo::WallCharger);
    }
    if (uPowerDevice->type() == 2) {
//battery
    }
}
void QBatteryInfoPrivate::uPowerBatteryPropertyChanged(const QString &prop, const QVariant &v)
{
    QUPowerDeviceInterface *uPowerDevice = qobject_cast<QUPowerDeviceInterface*>(sender());

    int foundBattery = 0;
    QMapIterator<int, QVariantMap> i(batteryMap);
    while (i.hasNext()) {
        i.next();
        if (i.value().value(QStringLiteral("NativePath")).toString() == uPowerDevice->nativePath()) {
            foundBattery = i.key();
            break;
        }
    }

    QVariantMap foundMap = batteryMap.value(foundBattery);
    foundMap.insert(prop,v);
    batteryMap.insert(foundBattery,foundMap);

    if (prop == QLatin1String("Energy")) {
        if (foundBattery == index)
            Q_EMIT remainingCapacityChanged(v.toDouble() * 1000);

    } else if (prop == QLatin1String("EnergyRate")) {
        if (foundBattery == index)
            Q_EMIT currentFlowChanged(v.toDouble() / (uPowerDevice->voltage() * 1000));

    } else if (prop == QLatin1String("Percentage")) {
        int level = v.toInt();
        //  Q_EMIT remainingCapacityChanged(foundBattery, level);

        QBatteryInfo::LevelStatus stat = QBatteryInfo::LevelUnknown;

        if (level < 3)
            stat = QBatteryInfo::LevelEmpty;
        else if (level < 11)
            stat = QBatteryInfo::LevelLow;
        else if (level < 99)
            stat = QBatteryInfo::LevelOk;
        else
            stat = QBatteryInfo::LevelFull;

        //   if (batteryMap.value(foundBattery).value(QStringLiteral("Percentage")).toInt() != stat) {
        if (foundBattery == index)
            Q_EMIT levelStatusChanged(stat);
        //   }

    } else if (prop == QLatin1String("Voltage")) {
        if (foundBattery == index)
            Q_EMIT voltageChanged(v.toDouble() * 1000 );

    } else if (prop == QLatin1String("State")) {

        QBatteryInfo::ChargingState curChargeState = getCurrentChargingState(v.toInt());

        if (curChargeState != cState) {
            cState = curChargeState;
            if (foundBattery == index)
                Q_EMIT chargingStateChanged(curChargeState);
        }

        } else if (prop == QLatin1String("Capacity")) {
        qDebug() << "Your battery just got less capacity";
    } else if (prop == QLatin1String("TimeToFull")) {
        if (foundBattery == index)
            Q_EMIT remainingChargingTimeChanged(v.toInt());

    } else if (prop == QLatin1String("Type")) {
        if (uPowerDevice->isOnline()) {
            QBatteryInfo::ChargerType curCharger = getChargerType(uPowerDevice->nativePath());
            if (curCharger != cType) {
                cType = curCharger;
                Q_EMIT chargerTypeChanged(cType);
            }
        }
    }
}

QBatteryInfo::ChargerType QBatteryInfoPrivate::getChargerType(const QString &path)
{
    QFile charger;
    QBatteryInfo::ChargerType chargerType = QBatteryInfo::UnknownCharger;
    charger.setFileName(path + QStringLiteral("/type"));
    if (charger.open(QIODevice::ReadOnly)) {
        QString line = QString::fromLocal8Bit(charger.readAll().simplified());
        if (line  == QStringLiteral("USB")) {
            chargerType = QBatteryInfo::USBCharger;

        } else if (line == QStringLiteral("Mains")) {
            chargerType = QBatteryInfo::WallCharger;
        }
    }
    charger.close();
    return chargerType;
}

QBatteryInfo::ChargingState QBatteryInfoPrivate::getCurrentChargingState(int state)
{
    QBatteryInfo::ChargingState curChargeState = QBatteryInfo::UnknownChargingState;
    switch (state) {
    case 1: // charging
    {
        curChargeState = QBatteryInfo::Charging;
    }
        break;
    case 2: //discharging
    case 3: //empty
        curChargeState = QBatteryInfo::Discharging;
        break;
    case 4: //fully charged
        curChargeState = QBatteryInfo::IdleChargingState;
        break;
    case 5: //pending charge
    case 6: //pending discharge
        break;
    default:
        curChargeState = QBatteryInfo::UnknownChargingState;
        break;
    };

    return curChargeState;
}

void QBatteryInfoPrivate::getBatteryStats()
{
    int batteryNumber = 0;
    batteryMap.clear();
    QUPowerInterface *power;
    power = new QUPowerInterface(this);

    connect(power,SIGNAL(deviceAdded(QString)),
            this,SLOT(deviceAdded(QString)));
    connect(power,SIGNAL(deviceRemoved(QString)),
            this,SLOT(deviceRemoved(QString)));

    foreach (const QDBusObjectPath &objpath, power->enumerateDevices()) {
        QUPowerDeviceInterface *battery;
        battery = new QUPowerDeviceInterface(objpath.path(),this);

        if (!battery->isPowerSupply())
            continue;
        if (battery->type() == 1) { //line power
            cType = getChargerType(battery->nativePath());
        }
        if (battery->type() == 2) { //battery power

            batteryMap.insert(batteryNumber++,battery->getProperties());

            connect(battery,SIGNAL(changed()),this,SLOT(upowerDeviceChanged()));
            connect(battery,SIGNAL(propertyChanged(QString,QVariant)),
                    this,SLOT(uPowerBatteryPropertyChanged(QString,QVariant)));

            cState = getCurrentChargingState(battery->state());

        } //end enumerateDevices
    }
}

void QBatteryInfoPrivate::deviceAdded(const QString &path)
{
    QUPowerDeviceInterface *battery;
    battery = new QUPowerDeviceInterface(path,this);
    int batteryNumber = batteryCount();

    if (battery->type() == 2) {
        batteryMap.insert(++batteryNumber,battery->getProperties());
        connect(battery,SIGNAL(changed()),this,SLOT(upowerDeviceChanged()));
        connect(battery,SIGNAL(propertyChanged(QString,QVariant)),
                this,SLOT(uPowerBatteryPropertyChanged(QString,QVariant)));

    }
}

void QBatteryInfoPrivate::deviceRemoved(const QString &path)
{
    QUPowerDeviceInterface *battery;
    battery = new QUPowerDeviceInterface(path,this);

    int foundBattery = 0;
    QMapIterator<int, QVariantMap> i(batteryMap);
     while (i.hasNext()) {
         i.next();
         if (i.value().value(QStringLiteral("NativePath")).toString()
                 == battery->nativePath()) {
             foundBattery = i.key();
             break;
         }
     }

    bool validBefore = isValid();
    if (battery->type() == 2) {
        batteryMap.remove(foundBattery);
        disconnect(battery,SIGNAL(changed()),this,SLOT(upowerDeviceChanged()));
        disconnect(battery,SIGNAL(propertyChanged(QString,QVariant)),
                this,SLOT(uPowerBatteryPropertyChanged(QString,QVariant)));
    }
    bool validNow = isValid();
    if (validBefore != validNow)
        Q_EMIT validChanged(validNow);
}

QT_END_NAMESPACE

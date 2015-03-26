/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the examples of the Qt Mobility Components.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>

#include "voipdialer.h"

VoipDialer::VoipDialer(QObject *parent)
    : QObject(parent), timerId(0), m_state(Disconnected)
{
    setObjectName("VoipService");
    qsrand(QTime(0,0,0).secsTo(QTime::currentTime())+QCoreApplication::applicationPid());
}

VoipDialer::ConnectionState VoipDialer::state() const
{
    return m_state;
}

void VoipDialer::dialNumber(const QString& number)
{
    qDebug() << "Dialing Voip number: " << number;
    if (m_state != Disconnected)
        return;

    if (timerId)
        killTimer(timerId);
    timerId = startTimer(2000);
    m_state = Connecting;
    emit stateChanged();
}

void VoipDialer::timerEvent(QTimerEvent* /*event*/)
{
    setNewState();
}

void VoipDialer::hangup()
{
    qDebug() << "Hangup on VoipDialer";
    if (timerId)
        killTimer(timerId);
    timerId = 0;
    m_state = Disconnected;
    emit stateChanged();
}

void VoipDialer::setNewState()
{

    switch (m_state) {
        case Disconnected:
            break;
        case Connecting:
            m_state = Connected;
            emit stateChanged();
            break;
        case Connected:
            break;
        case Engaged:

            break;
    }
}

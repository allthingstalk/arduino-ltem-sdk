/*
 * Copyright (c) 2019 Gabriel Notman.  All rights reserved.
 *
 * This file is part of Sodaq_R4X_MQTT.
 *
 * Sodaq_R4X_MQTT is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of
 * the License, or(at your option) any later version.
 *
 * Sodaq_R4X_MQTT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Sodaq_R4X_MQTT.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#include "Sodaq_R4X_MQTT.h"

void Sodaq_R4X_MQTT::setR4Xinstance(Sodaq_R4X* r4xInstance, bool (*r4xConnectHandler)(void))
{
    _r4xInstance = r4xInstance;
    _r4xConnectHandler = r4xConnectHandler;    
}

bool Sodaq_R4X_MQTT::openMQTT(const char * server, uint16_t port)
{
    if (_r4xConnectHandler && (!_r4xConnectHandler())) {
        return false;
    }

    if (_r4xInstance) {
        _socketID = _r4xInstance->socketCreate(0, Protocols::TCP);
        if (_socketID >= 0) {
            //_r4xInstance->socketSetR4Option(_socketID, 65535, 8, 1);
            return _r4xInstance->socketConnect(_socketID, server, port);
        }
    }

    return false;
}

bool Sodaq_R4X_MQTT::closeMQTT(bool switchOff)
{
    if (isAliveMQTT()) {
        _r4xInstance->socketClose(_socketID, true);
    }

    if (_r4xInstance && switchOff) {
        _r4xInstance->off();
    }

    // Always reset the socketID
    _socketID = -1;

    // Always return true
    return true;
}

bool Sodaq_R4X_MQTT::sendMQTTPacket(uint8_t* pckt, size_t pckt_len)
{
    if (isAliveMQTT()) {
        //_r4xInstance->execCommand("AT+USOGO=0,65535,8");
        return (_r4xInstance->socketWrite(_socketID, pckt, pckt_len) > 0);
        //&& (_r4xInstance->socketFlush(_socketID)));
    }

    return false;
}

size_t Sodaq_R4X_MQTT::receiveMQTTPacket(uint8_t * pckt, size_t size, uint32_t timeout)
{
    if (isAliveMQTT()) {
        if (_r4xInstance->socketWaitForRead(_socketID, timeout)) {
            return _r4xInstance->socketRead(_socketID, pckt, size);
        }
    }

    return 0;
}

size_t Sodaq_R4X_MQTT::availableMQTTPacket()
{
    if (isAliveMQTT()) {
        _r4xInstance->mqttLoop();
        return _r4xInstance->socketGetPendingBytes(_socketID);
    }

    return 0;
}

bool Sodaq_R4X_MQTT::isAliveMQTT()
{
    if ((_r4xInstance) && (_socketID >= 0) && (!_r4xInstance->socketIsClosed(_socketID))) {
        return true;
    }

    mqtt.setStateClosed();
    _socketID = -1;

    return false;
}

void Sodaq_R4X_MQTT::setMQTTClosedHandler(void (*handler)(void)) 
{
}

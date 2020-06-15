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

#ifndef Sodaq_R4X_MQTT_H
#define Sodaq_R4X_MQTT_H

#include <Arduino.h>
#include <stdint.h>
#include <Sodaq_MQTT.h>
#include <Sodaq_R4X.h>
#include "Sodaq_MQTT_Interface.h"

class Sodaq_R4X_MQTT : public Sodaq_MQTT_Interface {
public:
    // Set R4X instance
    void setR4Xinstance(Sodaq_R4X* r4xInstance, bool (*r4xConnectHandler)(void));

    // MQTT
    bool openMQTT(const char * server, uint16_t port = 1883);
    bool closeMQTT(bool switchOff=true);
    bool sendMQTTPacket(uint8_t * pckt, size_t len);
    size_t receiveMQTTPacket(uint8_t * pckt, size_t size, uint32_t timeout = 20000);
    size_t availableMQTTPacket();
    bool isAliveMQTT();
    void setMQTTClosedHandler(void (*handler)(void));

private:
    Sodaq_R4X* _r4xInstance = NULL;
    int8_t _socketID = -1;

    bool (*_r4xConnectHandler)(void);
};

#endif // Sodaq_R4X_MQTT_H

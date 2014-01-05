/* Universal Socket Modem Interface Library
* Copyright (c) 2013 Multi-Tech Systems
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#ifndef TESTPING_H
#define TESTPING_H

#include "mbed.h"
#include "include_me.h"

#define MAX_TRIES 5
#define MAX_REGISTRATION_TRIES 10

// 0 for shield board with wifi
// 1 for shield board with cellular
#define CELL_SHIELD 0

/* tries to ping 8.8.8.8 (Google's DNS server)
 * blinks green LED if successful, red LED if failure */

using namespace mts;

bool cellPingTest(const std::string& apn, const std::string& server);
bool wifiPingTest(const std::string& server, const std::string& ssid, Wifi::SecurityType type, const std::string& key);
void blinkLed(DigitalOut led);

void testPing() {
    DigitalOut ledG(LED1);
    DigitalOut ledR(LED2);
    
    ledG = 1;
    ledR = 1;
    
    std::string server = "8.8.8.8"; // Google's DNS server
#if CELL_SHIELD
    std::string apn = "wap.cingular"; // APN of sim card
    if (cellPingTest(apn, server)) {
#else
    std::string ssid = ""; // ssid of wireless network
    Wifi::SecurityType type = Wifi::WPA2; // NONE, WEP64, WEP128, WPA, WPA2
    std::string key = ""; // password for network (if type is not "NONE")
    if (wifiPingTest(server, ssid, type, key)) {
#endif
        printf("success!\n\r");
        blinkLed(ledG);
    } else {
        printf("failure!\n\r");
        blinkLed(ledR);
    }
}

bool wifiPingTest(const std::string& server, const std::string& ssid, Wifi::SecurityType type, const std::string& key) {
    int i;
    
    for (int i = 6; i >= 0; i = i - 2) {
        wait(2);
        printf("Waiting %d seconds...\n\r", i);
    }
    
    Transport::setTransport(Transport::WIFI);
    MTSSerial* serial = new MTSSerial(PTD3, PTD2, 256, 256);
    serial->baud(9600);
    Wifi* wifi = Wifi::getInstance();
    wifi->init(serial);
    
    i = 0;
    while (i++ < MAX_TRIES) {
        if (wifi->setNetwork(ssid, type, key) == SUCCESS) {
            printf("set network\r\n");
            break;
        } else {
            printf("failed to set network\r\n");
        }
        wait(1);
    }
    
    i = 0;
    while (i++ < MAX_TRIES) {
        if (wifi->setDeviceIP() == SUCCESS) {
            printf("set IP\r\n");
            break;
        } else {
            printf("failed to set IP\r\n");
        }
        wait(1);
    }
        
    i = 0;
    while (i++ < MAX_TRIES) {
        if (wifi->connect()) {
            printf("connected\r\n");
            break;
        } else {
            printf("failed to connect\r\n");
        }
        wait(1);
    }
    
    printf("pinging %s\n\r", server.c_str());
    return wifi->ping(server);
}

bool cellPingTest(const std::string& apn, const std::string& server) {
    int i;
    
    
    for (int i = 30; i >= 0; i = i - 2) {
        wait(2);
        printf("Waiting %d seconds...\n\r", i);
    }  
    
    Transport::setTransport(Transport::CELLULAR);
    MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
    serial->baud(115200);
    Cellular* cell = Cellular::getInstance();
    cell->init(serial);
    
    i = 0;
    while (i++ < MAX_REGISTRATION_TRIES) {
        if (cell->getRegistration() == Cellular::REGISTERED) {
            printf("registered with tower\n\r");
            break;
        } else if (i >= MAX_REGISTRATION_TRIES) {
            printf("failed to register with tower\n\r");
            return false;
        }
        wait(3);
    }
    
    i = 0;
    printf("setting APN to %s\n\r", apn.c_str());
    while (i++ < MAX_TRIES) {
        if (cell->setApn(apn) == SUCCESS) {
            printf("successfully set APN\n\r");
            break;
        } else if (i >= MAX_TRIES) {
            printf("failed to set APN\n\r");
            return false;
        }
        wait(1);
    }
    
    i = 0;
    printf("bringing up PPP link\n\r");
    while (i++ < MAX_TRIES) {
        if (cell->connect()) {
            printf("PPP link is up\n\r");
            break;
        } else if (i >= MAX_TRIES) {
            printf("failed to bring PPP link up\n\r");
            return false;
        }
        wait(1);
    }
    
    printf("pinging %s\n\r", server.c_str());
    return cell->ping(server);
}

void blinkLed(DigitalOut led) {
    led = 0;
    
    while (true) {
        wait(0.25);
        led = !led;
    }
}

#endif

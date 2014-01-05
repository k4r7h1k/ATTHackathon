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

#include "Wifi.h"
#include "MTSText.h"

#if 0
//Enable debug
#include <cstdio>
#define DBG(x, ...) std::printf("Line: %d %s \t[Wifi : DBG]"x"\r\n", __LINE__, __FILE__, ##__VA_ARGS__);
#else
#define DBG(x, ...)
#endif

Wifi* Wifi::instance = NULL;

Wifi* Wifi::getInstance()
{
    if(instance == NULL) {
        instance = new Wifi(NULL);
    }
    return instance;
}

bool Wifi::sortInterfaceMode(void)
{
    //Check initial state of command mode
    std::string response = sendCommand("", 1000, ">");
    if(response.find(">") != string::npos) {
        cmdOn = true;
    }

    //Set device into command mode
    if (!setCmdMode(true)) {
        return false;
    }

    return true;
}

bool Wifi::init(MTSBufferedIO* io)
{
    if (io == NULL) {
        return false;
    }
    instance->io = io;

    // start from the same place each time
    reset();

    //Secure interface mode
    if(!sortInterfaceMode()) {
        return false;
    }

    //Set device to non-echo mode
    while (sendBasicCommand("set uart mode 1", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set to non-echo mode\n\r");
        //return false;
    }
    // do this twice because the module response doesnt seem to always take
    while (sendBasicCommand("set uart mode 1", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set to non-echo mode\n\r");
        //return false;
    }

    //Set device to manual infrastructure mode
    if (sendBasicCommand("set wlan join 0", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set join mode\n\r");
        return false;
    }

    //Set device to channel auto-scanning mode
    if (sendBasicCommand("set wlan channel 0", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set auto-scanning mode\n\r");
        return false;
    }

    //Set device so no data is transmitted immediately following a socket connection
    if (sendBasicCommand("set comm remote 0", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set remote transmit mode\n\r");
        return false;
    }

    //Set device into DHCP mode by default
    if (sendBasicCommand("set ip dhcp 1", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set to default DHCP mode\n\r");
        return false;
    }

    return true;
}

Wifi::Wifi(MTSBufferedIO* io)
    : io(io)
    , wifiConnected(false)
    , _ssid("")
    , mode(TCP)
    , socketOpened(false)
    , socketCloseable(true)
    , local_port(0)
    , local_address("")
    , host_port(0)
    , cmdOn(false)
{

}

Wifi::~Wifi()
{
}

bool Wifi::connect()
{
    //Check if socket is open
    if(socketOpened) {
        return true;
    }

    //Run Test first to validate a good state
    if(isConnected()) {
        return true;
    }

    if (_ssid.size() == 0) {
        printf("[ERROR] No SSID has been set\n\r");
        return false;
    }

    if(!setCmdMode(true)) {
        return false;
    }

    //Possibly add a scan command here and look for the network....

    //join my_network
    printf("[DEBUG] Making SSID Connection Attempt. SSID[%s]\r\n", _ssid.c_str());
    std::string result = sendCommand("join " + _ssid, 15000, "GW=");
    //printf("Connect Status: %s\n\r", result.c_str());

    //Check whether connection was successful
    if(result.find("Associated!") != string::npos) {
        if(result.find("Static") == string::npos) {
            int start = result.find("IP=");
            int stop = result.find(":", start);
            local_address = result.substr(start + 3, stop - start - 3);
        }
        printf("[INFO] WiFi Connection Established: IP[%s]\r\n", local_address.c_str());
        wifiConnected = true;

        //Report Signal Strength of new connection
        wait(1); //Needed for signal strength to be available
        int rssi = getSignalStrength();
        printf("[DEBUG] Signal strength (dBm): %d\r\n", rssi);
    } else {
        wifiConnected = false;
    }

    return wifiConnected;
}

void Wifi::disconnect()
{
    wait(5.0f);
    printf("[DEBUG] Disconnecting from network\r\n");

    if(socketOpened) {
        close();
    }

    if(!setCmdMode(true)) {
        printf("[ERROR] Failed in disconnecting from network.  Continuing ...\r\n");
    }

    std::string response = sendCommand("leave", 10000, "<4.00>");
    response = sendCommand("show net", 5000, "Links");
    //printf("Response: %s\n\r", response.c_str());
    if (response.find("Assoc=FAIL") != string::npos) {
        printf("[DEBUG] Successfully disconnected from network\r\n");
    } else {
        printf("[ERROR] Failed in disconnecting from network.  Continuing ...\r\n");
    }

    wifiConnected = false;
}

bool Wifi::isConnected()
{
    //1) Check if SSID was set
    if(_ssid.size() == 0) {
        printf("[DEBUG] SSID is not set\r\n");
        return false;
    }

    //1) Check that we do not have a live connection up
    if(isOpen()) {
        printf("[DEBUG] Socket is opened\r\n");
        return true;
    }

    //Check command mode.
    if(!setCmdMode(true)) {
        return false;
    }

    //2) Query the wifi module
    wifiConnected = false;
    std::string result = sendCommand("show net", 5000, "Links");
    //printf("netResult: %s\n\r", result);
    if(result.find("Assoc=OK") != std::string::npos) {
        wifiConnected = true;
    }

    return wifiConnected;
}

bool Wifi::bind(unsigned int port)
{
    if(socketOpened) {
        printf("[ERROR] socket is open. Can not set local port\r\n");
        return false;
    }
    if(port > 65535) {
        printf("[ERROR] port out of range (0-65535)\r\n");
        return false;
    }
    local_port = port;
    return true;
}

bool Wifi::open(const std::string& address, unsigned int port, Mode mode)
{
    char buffer[256] = {0};
    printf("[DEBUG] Attempting to Open Socket\r\n");

    //1) Check that we do not have a live connection up
    if(socketOpened) {
        //Check that the address, port, and mode match
        if(host_address != address || host_port != port || this->mode != mode) {
            if(this->mode == TCP) {
                printf("[ERROR] TCP socket already opened (%s:%d)\r\n", host_address.c_str(), host_port);
            } else {
                printf("[ERROR] UDP socket already opened (%s:%d)\r\n", host_address.c_str(), host_port);
            }
            return false;
        }

        printf("[DEBUG] Socket already opened\r\n");
        return true;
    }

    //2) Check Parameters
    if(port > 65535) {
        printf("[ERROR] port out of range (0-65535)\r\n");
        return false;
    }


    //3) Check Wifi network connection
    if(!isConnected()) {
        printf("[ERROR] Wifi network not connected.  Attempting to connect\r\n");
        if(!connect()) {
            printf("[ERROR] Wifi network connection failed\r\n");
            return false;
        } else {
            printf("[DEBUG] Wifi connection established\r\n");
        }
    }

    //Check command mode
    if(!setCmdMode(true)) {
        return false;
    }

    //Set Local Port
    if(local_port != 0) {
        //Attempt to set local port
        sprintf(buffer, "set ip localport %d", local_port);
        Code code = sendBasicCommand(buffer, 1000);
        if(code != SUCCESS) {
            printf("[WARNING] Unable to set local port (%d) [%d]. Continuing...\r\n", local_port, (int) code);
        }
    }

    //Set TCP/UDP parameters
    sprintf(buffer, "set ip remote %d", port);
    if(sendBasicCommand(buffer, 1000) == SUCCESS) {
        host_port = port;
    } else {
        printf("[ERROR] Host port could not be set\r\n");
    }

    //Check if address of URL
    std::vector<std::string> tmp = Text::split(address, '.');
    if(tmp.size() != 4) {
        std::string ip = getHostByName(address);
        if(ip.size() != 0) {
            host_address = ip;
        } else {
            return false;
        }
    } else {
        host_address = address;
    }

    //Set Address
    printf("[DEBUG] Host address: %s\n\r", host_address.c_str());
    if(sendBasicCommand("set ip host " + host_address, 1000) != SUCCESS) {
        printf("[ERROR] Host address could not be set\r\n");
        return false;
    }

    if(sendBasicCommand("set ip protocol 8", 1000) != SUCCESS) {
        printf("[ERROR] Failed to set TCP mode\r\n");
        return false;
    }

    // Try and Connect
    std::string sMode;
    std::string sOpenSocketCmd;
    if(mode == TCP) {
        sOpenSocketCmd = "open";
        sMode = "TCP";
    } else {
        //TODO
        //sOpenSocketCmd = "AT#OUDP";
        //sMode = "UDP";
    }
    string response = sendCommand(sOpenSocketCmd, 10000, "OPEN");
    if (response.find("OPEN") != string::npos) {
        printf("[INFO] Opened %s Socket [%s:%d]\r\n", sMode.c_str(), host_address.c_str(), port);
        socketOpened = true;
        cmdOn = false;
    } else {
        printf("[WARNING] Unable to open %s Socket [%s:%d]\r\n", sMode.c_str(),  host_address.c_str(), port);
        socketOpened = false;
    }

    return socketOpened;
}

bool Wifi::isOpen()
{
    if(io->readable()) {
        printf("[DEBUG] Assuming open, data available to read.\n\r");
        return true;
    }
    if(!setCmdMode(true)) {
        printf("[ERROR] Failed to properly check if TCP connection is open.\r\n");
        return socketOpened;
    }
    std::string response = sendCommand("show connection", 2000, "\n");
    int start = response.find("f");
    if(start != string::npos && response.size() >= (start + 3)) {
        if(response[start + 3] == '1') {
            socketOpened = true;
        } else {
            socketOpened = false;
        }
    } else {
        printf("[WARNING] Trouble checking TCP Connection status.\n\r");
    }
    return socketOpened;
}

bool Wifi::close()
{
    wait(1);
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return false;
    }

    if(!socketOpened) {
        printf("[WARNING] Socket close() called, but socket was not open\r\n");
        return true;
    }

    if(!setCmdMode(true)) {
        printf("[ERROR] Failed to close socket\r\n");
        return false;
    }

    if(isOpen()) {
        std::string response = sendCommand("close", 3000, "CLOS");
        if(response.find("CLOS") == string::npos) {
            printf("[WARNING] Failed to successfully close socket...\r\n");
            return false;
        }
    }

    wait(1); //Wait so the subsequent isOpen calls return correctly.
    io->rxClear();
    io->txClear();

    return true;
}

int Wifi::read(char* data, int max, int timeout)
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return -1;
    }

    //Check that nothing is in the rx buffer
    if(!socketOpened && !io->readable()) {
        printf("[ERROR] Socket is not open\r\n");
        return -1;
    }

    //Check for data mode
    if(!setCmdMode(false)) {
        printf("[ERROR] Failed to read data due to mode\r\n");
        return -1;
    }

    int bytesRead = 0;

    if(timeout >= 0) {
        bytesRead = io->read(data, max, static_cast<unsigned int>(timeout));
    } else {
        bytesRead = io->read(data, max);
    }

    return bytesRead;
}

int Wifi::write(const char* data, int length, int timeout)
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return -1;
    }

    if(!socketOpened) {
        printf("[ERROR] Socket is not open\r\n");
        return -1;
    }

    //Check for data mode
    if(!setCmdMode(false)) {
        printf("[ERROR] Failed to write data due to mode\r\n");
        return -1;
    }

    int bytesWritten = 0;

    if(timeout >= 0) {
        bytesWritten = io->write(data, length, static_cast<unsigned int>(timeout));
    } else {
        bytesWritten = io->write(data, length);
    }

    return bytesWritten;
}

unsigned int Wifi::readable()
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return 0;
    }
    if(!socketOpened) {
        printf("[ERROR] Socket is not open\r\n");
        return 0;
    }
    return io->readable();
}

unsigned int Wifi::writeable()
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return 0;
    }
    if(!socketOpened) {
        printf("[ERROR] Socket is not open\r\n");
        return 0;
    }

    return io->writeable();
}

void Wifi::reset()
{
    if(!sortInterfaceMode()) {
        return;
    }

    sendCommand("factory RESET", 2000, "Set Factory Default"); // <ver> comes out about 1 sec later
    wait(0.5f);
    sendCommand("reboot", 2000, "*READY*");

    wifiConnected = false;
    _ssid = "";
    mode = TCP;
    socketOpened = false;
    socketCloseable = true;
    local_port = 0;
    local_address = "";
    host_port = 0;
    cmdOn = false;
    wait(1);
//    if(!init(io)) {
//        printf("[ERROR] Failed to reinitialize after reset.\n\r");
//    }
}

Code Wifi::setDeviceIP(std::string address)
{
    //Check for command mode
    if(!setCmdMode(true)) {
        printf("[ERROR] Failed to set IP due to mode issue\r\n");
        return FAILURE;
    }

    //Set to DHCP mode
    if(address.compare("DHCP") == 0) {
        return sendBasicCommand("set ip dhcp 1", 1000);
    }

    //Set to static mode and set address
    Code code = sendBasicCommand("set ip address " + address, 1000);
    if(code != SUCCESS) {
        return code;
    }
    code = sendBasicCommand("set ip dhcp 0", 1000);
    if(code != SUCCESS) {
        return code;
    }
    local_address = address;
    return SUCCESS;
}

std::string Wifi::getDeviceIP()
{
    return local_address;
}

Code Wifi::setNetwork(const std::string& ssid, SecurityType type, const std::string& key)
{
    //Check the command mode
    if(!setCmdMode(true)) {
        return FAILURE;
    }

    Code code;

    //Set the appropraite SSID
    code = sendBasicCommand("set wlan ssid " + ssid, 1000);
    if (code != SUCCESS) {
        return code;
    }

    //Set the security key
    if (type == WEP64 || type == WEP128) {
        //Set the WEP key if using WEP encryption
        code = sendBasicCommand("set wlan key " + key, 1000);
        if (code != SUCCESS) {
            return code;
        }
    } else if (type == WPA || type == WPA2) {
        //Set the WPA key if using WPA encryption
        code = sendBasicCommand("set wlan phrase " + key, 1000);
        if (code != SUCCESS) {
            return code;
        }
    }

    _ssid = ssid;
    return SUCCESS;
}

Code Wifi::setDNS(const std::string& dnsName)
{
    //Check the command mode
    if(!setCmdMode(true)) {
        return FAILURE;
    }

    return sendBasicCommand("set dns name " + dnsName, 1000);
}

int Wifi::getSignalStrength()
{
    //Signal strength does not report correctly if not connected
    if(!wifiConnected) {
        printf("[ERROR] Could not get RSSI, Wifi network not connected.\n\r");
        return 99;
    }

    //Check the command mode
    if(!setCmdMode(true)) {
        printf("[ERROR] Could not get RSSI\n\r");
        return 99;
    }

    string response = sendCommand("show rssi", 2000, "dBm");
    if (response.find("RSSI") == string::npos) {
        printf("[ERROR] Could not get RSSI\n\r");
        return 99;
    }
    int start = response.find('(');
    int stop = response.find(')', start);
    string signal = response.substr(start + 1, stop - start - 1);
    int value;
    sscanf(signal.c_str(), "%d", &value);
    return value;
}

bool Wifi::ping(const std::string& address)
{
    //Check the command mode
    if(!setCmdMode(true)) {
        printf("[ERROR] Could not send ping command\n\r");
        return false;
    }

    std::string response;
    for (int i = 0; i < PINGNUM; i++) {
        response = sendCommand("ping " + address, PINGDELAY * 1000, "reply");
        if (response.find("reply") != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool Wifi::setCmdMode(bool on)
{
    if (on) {
        if (cmdOn) {
            return true;
        }
        wait(.5);
        std::string response = sendCommand("$$", 2000, "CMD", '$');
        if (response.find("CMD") != string::npos) {
            cmdOn = true;
            wait(.5);
            return true;
        }
        printf("[ERROR] Failed to enter command mode\n\r");
        return false;
    } else {
        if (!cmdOn) {
            return true;
        }
        std::string response = sendCommand("exit", 2000, "EXIT");
        if (response.find("EXIT") != string::npos) {
            cmdOn = false;
            return true;
        }
        printf("[ERROR] Failed to exit command mode\n\r");
        return false;
    }
}

std::string Wifi::getHostByName(std::string url)
{
    std::string response = sendCommand("lookup " + url, 3000, "<4.00>");
    int start = response.find("=");
    int stop = response.find("\r");
    if(start == string::npos || stop == string::npos) {
        printf("[ERROR] Failed to resolve URL [%s]", response.c_str());
        return "";
    }
    std::string ip = response.substr(start + 1, stop - start - 1);
    //printf("Data: %s\n\r", ip);
    return ip;
}

Code Wifi::sendBasicCommand(string command, int timeoutMillis, char esc)
{
    if(socketOpened) {
        printf("[ERROR] socket is open. Can not send AT commands\r\n");
        return ERROR;
    }

    string response = sendCommand(command, timeoutMillis, "AOK", esc);
    //printf("Response: %s\n\r", response.c_str());
    if (response.size() == 0) {
        return NO_RESPONSE;
    } else if (response.find("AOK") != string::npos) {
        return SUCCESS;
    } else if (response.find("ERR") != string::npos) {
        return ERROR;
    } else {
        return FAILURE;
    }
}

string Wifi::sendCommand(string command, int timeoutMillis, std::string response, char esc)
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return "";
    }
    //if(socketOpened && command.compare("$$") != 0 && command.compare("exit") != 0 && command.compare("close") != 0) {
    //    printf("[ERROR] socket is open. Can not send AT commands\r\n");
    //    return "";
    //}

    io->rxClear();
    io->txClear();
    std::string result;

    //Attempt to write command
    if(io->write(command.data(), command.size(), timeoutMillis) != command.size()) {
        //Failed to write command
        printf("[ERROR] failed to send command to radio within %d milliseconds\r\n", timeoutMillis);
        return "";
    }
    //Send Escape Character
    if (esc != 0x00) {
        if(io->write(esc, timeoutMillis) != 1) {
            printf("[ERROR] failed to send '%c' to radio within %d milliseconds\r\n", esc, timeoutMillis);
            return "";
        }
    }
    DBG("Sending: %s%c", command.data(), esc);

    int timer = 0;
    size_t previous = 0;
    char tmp[256];
    tmp[255] = 0;
    bool done = false;
    do {
        wait(.2);
        timer = timer + 200;
        previous = result.size();
        int size = io->read(tmp, 255, 0);    //1 less than allocated
        if(size > 0) {
            result.append(tmp, size);
            if (response.size() != 0) {
                if (result.find(response) != string::npos) {
                    goto exit_func;
                    //return result;
                }
            } else {
                done =  (result.size() == previous);
            }
        }
        if(timer >= timeoutMillis) {
            if(!(command.compare("reboot") == 0 || command.compare("") == 0)) {
                printf("[WARNING] sendCommand [%s] timed out after %d milliseconds\r\n", command.c_str(), timeoutMillis);
            }
            done = true;
        }
    } while (!done);

exit_func:
    DBG("Result: %s\n\r", result.c_str());
    return result;
}


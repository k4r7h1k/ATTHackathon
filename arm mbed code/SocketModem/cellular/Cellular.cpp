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


#include "Cellular.h"
#include "MTSText.h"
#include "MTSSerial.h"

using namespace mts;

Cellular* Cellular::instance = NULL;

Cellular* Cellular::getInstance()
{
    if(instance == NULL) {
        instance = new Cellular(NULL);
    }
    return instance;
}

Cellular::Cellular(MTSBufferedIO* io)
    : io(io)
    , echoMode(true)
    , pppConnected(false)
    , mode(TCP)
    , socketOpened(false)
    , socketCloseable(true)
    , local_port(0)
    , local_address("")
    , host_port(0)
    , dcd(NULL)
    , dtr(NULL)
{
}

Cellular::~Cellular()
{
    if (dtr != NULL) {
        dtr->write(1);
    }

    delete dcd;
    delete dtr;
}

bool Cellular::init(MTSBufferedIO* io, PinName DCD, PinName DTR)
{
    if (io == NULL) {
        return false;
    }

    if(dcd) {
        delete dcd;
        dcd = NULL;
    }
    if(dtr) {
        delete dtr;
        dtr = NULL;
    }

    if (DCD != NC) {
        // the radio will raise and lower this line
        dcd = new DigitalIn(DCD); //PTA4 - KL46
    }
    if (DTR != NC) {
        dtr = new DigitalOut(DTR); //PTC9 - KL46
        /* This line should be lowered when we want to talk to the radio and raised when we're done
        for now we will lower it in the constructor and raise it in the destructor
        */
        dtr->write(0);
    }
    instance->io = io;

    return (test() == SUCCESS);
}


bool Cellular::connect()
{
    //Check if socket is open
    if(socketOpened) {
        return true;
    }

    //Check if already connected
    if(isConnected()) {
        return true;
    }

    Timer tmr;

    //Check Registration: AT+CREG? == 0,1
    tmr.start();
    do {
        Registration registration = getRegistration();
        if(registration != REGISTERED) {
            printf("[WARNING] Not Registered [%d] ... waiting\r\n", (int)registration);
            wait(1);
        } else {
            break;
        }
    } while(tmr.read() < 30);

    //Check RSSI: AT+CSQ
    tmr.reset();
    do {
        int rssi = getSignalStrength();
        printf("[DEBUG] Signal strength: %d\r\n", rssi);
        if(rssi == 99) {
            printf("[WARNING] No Signal ... waiting\r\n");
            wait(1);
        } else {
            break;
        }
    } while(tmr.read() < 30);

    //AT#CONNECTIONSTART: Make a PPP connection
    printf("[DEBUG] Making PPP Connection Attempt. APN[%s]\r\n", apn.c_str());
    std::string pppResult = sendCommand("AT#CONNECTIONSTART", 120000);
    std::vector<std::string> parts = Text::split(pppResult, "\r\n");

    if(pppResult.find("Ok_Info_GprsActivation") != std::string::npos) {
        if(parts.size() >= 2) {
            local_address = parts[1];
        }
        printf("[INFO] PPP Connection Established: IP[%s]\r\n", local_address.c_str());
        pppConnected = true;

    } else {
        pppConnected = false;
    }

    return pppConnected;
}

void Cellular::disconnect()
{
    //AT#CONNECTIONSTOP: Close a PPP connection
    printf("[DEBUG] Closing PPP Connection\r\n");

    if(socketOpened) {
        close();
    }

    Code code = sendBasicCommand("AT#CONNECTIONSTOP", 10000);
    if(code == SUCCESS) {
        printf("[DEBUG] Successfully closed PPP Connection\r\n");
    } else {
        printf("[ERROR] Closing PPP Connection [%d].  Continuing ...\r\n", (int)code);
    }

    pppConnected = false;
}

bool Cellular::isConnected()
{
    //1) Check if APN was set
    if(apn.size() == 0) {
        printf("[DEBUG] APN is not set\r\n");
        return false;
    }

    //1) Check that we do not have a live connection up
    if(socketOpened) {
        printf("[DEBUG] Socket is opened\r\n");
        return true;
    }
    //2) Query the radio
    std::string result = sendCommand("AT#VSTATE", 3000);
    if(result.find("CONNECTED") != std::string::npos) {
        if(pppConnected == false) {
            printf("[WARNING] Internal PPP state tracking differs from radio (DISCONNECTED:CONNECTED)\r\n");
        }
        pppConnected = true;
    } else {
        if(pppConnected == true) {
            //Find out what state is
            size_t pos = result.find("STATE:");
            if(pos != std::string::npos) {
                result = Text::getLine(result, pos + sizeof("STATE:"), pos);
                printf("[WARNING] Internal PPP state tracking differs from radio (CONNECTED:%s)\r\n", result.c_str());
            } else {
                printf("[ERROR] Unable to parse radio state: [%s]\r\n", result.c_str());
            }

        }
        pppConnected = false;
    }

    return pppConnected;
}

bool Cellular::bind(unsigned int port)
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

bool Cellular::open(const std::string& address, unsigned int port, Mode mode)
{
    char buffer[256] = {0};
    Code portCode, addressCode;

    //1) Check that we do not have a live connection up
    if(socketOpened) {
        //Check that the address, port, and mode match
        if(host_address != address || host_port != port || this->mode != mode) {
            if(this->mode == TCP) {
                printf("[ERROR] TCP socket already opened [%s:%d]\r\n", host_address.c_str(), host_port);
            } else {
                printf("[ERROR] UDP socket already opened [%s:%d]\r\n", host_address.c_str(), host_port);
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

    //3) Check PPP connection
    if(!isConnected()) {
        printf("[ERROR] PPP not established.  Attempting to connect\r\n");
        if(!connect()) {
            printf("[ERROR] PPP connection failed\r\n");
            return false;
        } else {
            printf("[DEBUG] PPP connection established\r\n");
        }
    }

    //Set Local Port
    if(local_port != 0) {
        //Attempt to set local port
        sprintf(buffer, "AT#OUTPORT=%d", local_port);
        Code code = sendBasicCommand(buffer, 1000);
        if(code != SUCCESS) {
            printf("[WARNING] Unable to set local port (%d) [%d]\r\n", local_port, (int) code);
        }
    }

    //Set TCP/UDP parameters
    if(mode == TCP) {
        if(socketCloseable) {
            Code code = sendBasicCommand("AT#DLEMODE=1,1", 1000);
            if(code != SUCCESS) {
                printf("[WARNING] Unable to set socket closeable [%d]\r\n", (int) code);
            }
        }
        sprintf(buffer, "AT#TCPPORT=1,%d", port);
        portCode = sendBasicCommand(buffer, 1000);
        addressCode = sendBasicCommand("AT#TCPSERV=1,\"" + address + "\"", 1000);
    } else {
        if(socketCloseable) {
            Code code = sendBasicCommand("AT#UDPDLEMODE=1", 1000);
            if(code != SUCCESS) {
                printf("[WARNING] Unable to set socket closeable [%d]\r\n", (int) code);
            }
        }
        sprintf(buffer, "AT#UDPPORT=%d", port);
        portCode = sendBasicCommand(buffer, 1000);
        addressCode = sendBasicCommand("AT#UDPSERV=\"" + address + "\"", 1000);
    }

    if(portCode == SUCCESS) {
        host_port = port;
    } else {
        printf("[ERROR] Host port could not be set\r\n");
    }

    if(addressCode == SUCCESS) {
        host_address = address;
    } else {
        printf("[ERROR] Host address could not be set\r\n");
    }

    // Try and Connect
    std::string sMode;
    std::string sOpenSocketCmd;
    if(mode == TCP) {
        sOpenSocketCmd = "AT#OTCP=1";
        sMode = "TCP";
    } else {
        sOpenSocketCmd = "AT#OUDP";
        sMode = "UDP";
    }

    string response = sendCommand(sOpenSocketCmd, 30000);
    if (response.find("Ok_Info_WaitingForData") != string::npos) {
        printf("[INFO] Opened %s Socket [%s:%d]\r\n", sMode.c_str(), address.c_str(), port);
        socketOpened = true;
    } else {
        printf("[WARNING] Unable to open %s Socket [%s:%d]\r\n", sMode.c_str(),  address.c_str(), port);
        socketOpened = false;
    }

    return socketOpened;
}

bool Cellular::isOpen()
{
    if(io->readable()) {
        printf("[DEBUG] Assuming open, data available to read.\n\r");
        return true;
    }
    return socketOpened;
}

bool Cellular::close()
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return false;
    }

    if(!socketOpened) {
        printf("[WARNING] Socket close() called, but socket was not open\r\n");
        return true;
    }

    if(!socketCloseable) {
        printf("[ERROR] Socket is not closeable\r\n");
        return false;
    }



    if(io->write(ETX, 1000) != 1) {
        printf("[ERROR] Timed out attempting to close socket\r\n");
        return false;
    }

    Timer tmr;
    int counter = 0;
    char tmp[256];
    tmr.start();
    do {
        if(socketOpened == false) {
            break;
        }
        read(tmp, 256, 1000);
    } while(counter++ < 10);

    io->rxClear();
    io->txClear();

    socketOpened = false;
    return true;
}

int Cellular::read(char* data, int max, int timeout)
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

    int bytesRead = 0;

    if(timeout >= 0) {
        bytesRead = io->read(data, max, static_cast<unsigned int>(timeout));
    } else {
        bytesRead = io->read(data, max);
    }

    if(bytesRead > 0 && socketCloseable) {
        //Remove escape characters
        int index = 0;
        bool escapeFlag = false;
        for(int i = 0; i < bytesRead; i++) {
            if(data[i] == DLE || data[i] == ETX) {
                if(escapeFlag == true) {
                    //This character has been escaped
                    escapeFlag = false;
                } else if(data[bytesRead] == DLE) {
                    //Found escape character
                    escapeFlag = true;
                    continue;
                } else {
                    //ETX sent without escape -> Socket closed
                    printf("[INFO] Read ETX character without DLE escape. Socket closed\r\n");
                    socketOpened = false;
                    continue;
                }
            }

            if(index != i) {
                data[index] = data[i];
            }
            index++;
        }
        bytesRead = index;
    }

    //Scan for socket closed message
    for(size_t i = 0; i < bytesRead; i++) {
        if(data[i] == 'O') {
            if(strstr(&data[i], "Ok_Info_SocketClosed")) {
                printf("[INFO] Found socket closed message. Socket closed\r\n");
                //Close socket and Cut Off End of Message
                socketOpened = false;
                data[i] = '\0';
                bytesRead = i;
                break;
            }
        }
    }
    return bytesRead;
}

int Cellular::write(const char* data, int length, int timeout)
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return -1;
    }

    if(!socketOpened) {
        printf("[ERROR] Socket is not open\r\n");
        return -1;
    }

    //In order to avoid allocating another buffer, capture indices of
    //characters to escape during write
    int specialWritten = 0;
    std::vector<int> vSpecial;
    if(socketCloseable) {
        for(int i = 0; i < length; i++) {
            if(data[i] == ETX || data[i] == DLE) {
                //Push back index of special characters
                vSpecial.push_back(i);
            }
        }
    }

    int bytesWritten = 0;
    if(timeout >= 0) {
        Timer tmr;
        tmr.start();
        do {
            int available = io->writeable();
            if (available > 0) {
                if(specialWritten < vSpecial.size()) {
                    //Check if current index is at a special character
                    if(bytesWritten == vSpecial[specialWritten]) {
                        if(available < 2) {
                            //Requires at least two bytes of space
                            wait(0.05);
                            continue;
                        }
                        //Ready to write special character
                        if(io->write(DLE)) {
                            specialWritten++;
                            if(io->write(data[bytesWritten])) {
                                bytesWritten++;
                            }
                        } else {
                            //Unable to write escape character, try again next round
                            wait(0.05);
                        }
                    } else {
                        //We want to write all the way up to the next special character
                        int relativeIndex = vSpecial[specialWritten] - bytesWritten;
                        int size = MIN(available, relativeIndex);
                        bytesWritten += io->write(&data[bytesWritten], size);
                    }
                } else {
                    int size = MIN(available, length - bytesWritten);
                    bytesWritten += io->write(&data[bytesWritten], size);
                }
            } else {
                wait(0.05);
            }
        } while (tmr.read_ms() <= timeout && bytesWritten < length);
    } else {
        for(int i = 0; i < vSpecial.size(); i++) {
            //Write up to the special character, then write the special character
            int size = vSpecial[i] - bytesWritten;
            int currentWritten = io->write(&data[bytesWritten], size);
            bytesWritten += currentWritten;
            if(currentWritten != size) {
                //Failed to write up to the special character.
                return bytesWritten;
            }
            if(io->write(DLE) && io->write(data[bytesWritten])) {
                bytesWritten++;
            } else {
                //Failed to write the special character.
                return bytesWritten;
            }
        }

        bytesWritten = io->write(&data[bytesWritten], length - bytesWritten);
    }

    return bytesWritten;
}

unsigned int Cellular::readable()
{
    if(io == NULL) {
        printf("[WARNING] MTSBufferedIO not set\r\n");
        return 0;
    }
    if(!socketOpened && !io->readable()) {
        printf("[WARNING] Socket is not open\r\n");
        return 0;
    }
    return io->readable();
}

unsigned int Cellular::writeable()
{
    if(io == NULL) {
        printf("[WARNING] MTSBufferedIO not set\r\n");
        return 0;
    }
    if(!socketOpened) {
        printf("[WARNING] Socket is not open\r\n");
        return 0;
    }

    return io->writeable();
}

void Cellular::reset()
{
    disconnect();
    Code code = sendBasicCommand("AT#RESET=0", 10000);
    if(code != SUCCESS) {
        printf("[ERROR] Socket Modem did not accept RESET command\n\r");
    } else {
        printf("[WARNING] Socket Modem is resetting, allow 30 seconds for it to come back\n\r");
    }
}

std::string Cellular::getDeviceIP()
{
    return local_address;
}

Code Cellular::test()
{
    bool basicRadioComms = false;
    Code code;
    Timer tmr;
    tmr.start();
    do {
        printf("[DEBUG] Attempting basic radio communication\r\n");
        code = sendBasicCommand("AT", 1000);
        if(code == SUCCESS) {
            basicRadioComms = true;
            break;
        } else {
            wait(1);
        }
    } while(tmr.read() < 15);

    if(!basicRadioComms) {
        printf("[ERROR] Unable to communicate with the radio\r\n");
        return FAILURE;
    }

    return SUCCESS;
}

Code Cellular::echo(bool state)
{
    Code code;
    if (state) {
        code = sendBasicCommand("ATE0", 1000);
        echoMode = (code == SUCCESS) ? false : echoMode;
    } else {
        code = sendBasicCommand("ATE1", 1000);
        echoMode = (code == SUCCESS) ? true : echoMode;
    }
    return code;
}

int Cellular::getSignalStrength()
{
    string response = sendCommand("AT+CSQ", 1000);
    if (response.find("OK") == string::npos) {
        return -1;
    }
    int start = response.find(':');
    int stop = response.find(',', start);
    string signal = response.substr(start + 2, stop - start - 2);
    int value;
    sscanf(signal.c_str(), "%d", &value);
    return value;
}

Cellular::Registration Cellular::getRegistration()
{
    string response = sendCommand("AT+CREG?", 5000);
    if (response.find("OK") == string::npos) {
        return UNKNOWN;
    }
    int start = response.find(',');
    int stop = response.find(' ', start);
    string regStat = response.substr(start + 1, stop - start - 1);
    int value;
    sscanf(regStat.c_str(), "%d", &value);
    switch (value) {
        case 0:
            return NOT_REGISTERED;
        case 1:
            return REGISTERED;
        case 2:
            return SEARCHING;
        case 3:
            return DENIED;
        case 4:
            return UNKNOWN;
        case 5:
            return ROAMING;
    }
    return UNKNOWN;
}

Code Cellular::setApn(const std::string& apn)
{
    Code code = sendBasicCommand("AT#APNSERV=\"" + apn + "\"", 1000);
    if (code != SUCCESS) {
        return code;
    }
    this->apn = apn;
    return code;
}


Code Cellular::setDns(const std::string& primary, const std::string& secondary)
{
    return sendBasicCommand("AT#DNS=1," + primary + "," + secondary, 1000);
}

bool Cellular::ping(const std::string& address)
{
    char buffer[256] = {0};
    Code code;

    code = sendBasicCommand("AT#PINGREMOTE=\"" + address + "\"", 1000);
    if (code != SUCCESS) {
        return false;
    }

    sprintf(buffer, "AT#PINGNUM=%d", 1);
    code = sendBasicCommand(buffer , 1000);
    if (code != SUCCESS) {
        return false;
    }

    sprintf(buffer, "AT#PINGDELAY=%d", PINGDELAY);
    code = sendBasicCommand(buffer , 1000);
    if (code != SUCCESS) {
        return false;
    }

    std::string response;
    for (int i = 0; i < PINGNUM; i++) {
        response = sendCommand("AT#PING", PINGDELAY * 1000);
        if (response.find("alive") != std::string::npos) {
            return true;
        }
    }
    return false;
}

Code Cellular::setSocketCloseable(bool enabled)
{
    if(socketCloseable == enabled) {
        return SUCCESS;
    }

    if(socketOpened) {
        printf("[ERROR] socket is already opened. Can not set closeable\r\n");
        return ERROR;
    }

    socketCloseable = enabled;

    return SUCCESS;
}

Code Cellular::sendSMS(const Sms& sms)
{
    return sendSMS(sms.phoneNumber, sms.message);
}

Code Cellular::sendSMS(const std::string& phoneNumber, const std::string& message)
{
    Code code = sendBasicCommand("AT+CMGF=1", 1000);
    if (code != SUCCESS) {
        return code;
    }
    string cmd = "AT+CMGS=\"+";
    cmd.append(phoneNumber);
    cmd.append("\"");
    string response1 = sendCommand(cmd, 1000);
    if (response1.find('>') == string::npos) {
        return NO_RESPONSE;
    }
    wait(.2);
    string  response2 = sendCommand(message, 4000, CTRL_Z);
    printf("SMS Response: %s\r\n", response2.c_str());
    if (response2.find("+CMGS:") == string::npos) {
        return FAILURE;
    }
    return SUCCESS;
}

std::vector<Cellular::Sms> Cellular::getReceivedSms()
{
    int smsNumber = 0;
    std::vector<Sms> vSms;
    std::string received = sendCommand("AT+CMGL=\"ALL\"", 4000);
    size_t pos = received.find("+CMGL: ");

    while (pos != std::string::npos) {
        Cellular::Sms sms;
        std::string line(Text::getLine(received, pos, pos));
        //printf("[DEBUG] Top of SMS Parse Loop. LINE[%s]\r\n", line.c_str());
        if(line.find("+CMGL: ") == std::string::npos) {
            continue;
        }

        //Start of SMS message
        std::vector<std::string> vSmsParts = Text::split(line, ',');
        if(vSmsParts.size() != 6) {
            printf("[WARNING] Expected 6 commas. SMS[%d] DATA[%s]. Continuing ...\r\n", smsNumber, line.c_str());
            continue;
        }

        sms.phoneNumber = vSmsParts[2];
        sms.timestamp = vSmsParts[4] + ", " + vSmsParts[5];

        if(pos == std::string::npos) {
            printf("[WARNING] Expected SMS body. SMS[%d]. Leaving ...\r\n", smsNumber);
            break;
        }
        //Check for the start of the next SMS message
        size_t bodyEnd = received.find("\r\n+CMGL: ", pos);
        if(bodyEnd == std::string::npos) {
            //printf("[DEBUG] Parsing Last SMS. SMS[%d]\r\n", smsNumber);
            //This must be the last SMS message
            bodyEnd = received.find("\r\n\r\nOK", pos);
        }

        //Safety check that we found the boundary of this current SMS message
        if(bodyEnd != std::string::npos) {
            sms.message = received.substr(pos, bodyEnd - pos);
        } else {
            sms.message = received.substr(pos);
            printf("[WARNING] Expected to find end of SMS list. SMS[%d] DATA[%s].\r\n", smsNumber, sms.message.c_str());
        }
        vSms.push_back(sms);
        pos = bodyEnd;
        //printf("[DEBUG] Parsed SMS[%d].  Starting Next at position [%d]\r\n", smsNumber, pos);
        smsNumber++;
    }
    printf("Received %d SMS\r\n", smsNumber);
    return vSms;
}

Code Cellular::deleteOnlyReceivedReadSms()
{
    return sendBasicCommand("AT+CMGD=1,1", 1000);
}

Code Cellular::deleteAllReceivedSms()
{
    return sendBasicCommand("AT+CMGD=1,4", 1000);
}

Code Cellular::sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(socketOpened) {
        printf("[ERROR] socket is open. Can not send AT commands\r\n");
        return ERROR;
    }

    string response = sendCommand(command, timeoutMillis, esc);
    if (response.size() == 0) {
        return NO_RESPONSE;
    } else if (response.find("OK") != string::npos) {
        return SUCCESS;
    } else if (response.find("ERROR") != string::npos) {
        return ERROR;
    } else {
        return FAILURE;
    }
}

string Cellular::sendCommand(const std::string& command, unsigned int timeoutMillis, char esc)
{
    if(io == NULL) {
        printf("[ERROR] MTSBufferedIO not set\r\n");
        return "";
    }
    if(socketOpened) {
        printf("[ERROR] socket is open. Can not send AT commands\r\n");
        return "";
    }

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
            printf("[ERROR] failed to send character '%c' (0x%02X) to radio within %d milliseconds\r\n", esc, esc, timeoutMillis);
            return "";
        }
    }

    int timer = 0;
    size_t previous = 0;
    char tmp[256];
    tmp[255] = 0;
    bool started = !echoMode;
    bool done = false;
    do {
        wait(0.1);
        timer += 100;

        previous = result.size();
        //Make a non-blocking read call by passing timeout of zero
        int size = io->read(tmp,255,0);    //1 less than allocated (timeout is instant)
        if(size > 0) {
            result.append(tmp, size);
        }
        if(!started) {
            //In Echo Mode (Command will have echo'd + 2 characters for \r\n)
            if(result.size() > command.size() + 2) {
                started = true;
            }
        } else {
            done =  (result.size() == previous);
        }
        if(timer >= timeoutMillis) {
            printf("[WARNING] sendCommand [%s] timed out after %d milliseconds\r\n", command.c_str(), timeoutMillis);
            done = true;
        }
    } while (!done);

    return result;
}

std::string Cellular::getRegistrationNames(Registration registration)
{
    switch(registration) {
        case NOT_REGISTERED:
            return "NOT_REGISTERED";
        case REGISTERED:
            return "REGISTERED";
        case SEARCHING:
            return "SEARCHING";
        case DENIED:
            return "DENIED";
        case UNKNOWN:
            return "UNKNOWN";
        case ROAMING:
            return "ROAMING";
        default:
            return "UNKNOWN ENUM";
    }
}

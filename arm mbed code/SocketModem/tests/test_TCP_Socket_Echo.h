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

#ifndef _TEST_TCP_SOCKET_ECHO_H_
#define _TEST_TCP_SOCKET_ECHO_H_

// 0 for shield board with wifi
// 1 for shield board with cellular
#define CELL_SHIELD 0

/* test TCP socket communication
 * designed to talk to remote echo server
 * will talk to server until echo doesn't match sent data */
//Setup a netcat server with command: ncat -l 5798 -k -c 'xargs -n1 --null echo'

using namespace mts;

bool testTcpSocketEchoLoop();

void testTcpSocketEcho() {
    Code code;
    const int TEST_PORT = 5798;
    const std::string TEST_SERVER( /* public IP of server running the netcat command given above */);
    
    printf("TCP SOCKET TESTING\r\n");
#if CELL_SHIELD
    for (int i = 30; i >= 0; i = i - 2) {
        wait(2);
        printf("Waiting %d seconds...\n\r", i);
    }  
    Transport::setTransport(Transport::CELLULAR);
    MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
    serial->baud(115200);
    Cellular::getInstance()->init(serial);
    
    printf("Setting APN\r\n");
    code = Cellular::getInstance()->setApn("wap.cingular");
    if(code == SUCCESS) {
        printf("Success!\r\n");
    } else {
        printf("Error during APN setup [%d]\r\n", (int)code);
    }
#else
    for (int i = 6; i >= 0; i = i - 2) {
        wait(2);
        printf("Waiting %d seconds...\n\r", i);
    }  
    Transport::setTransport(Transport::WIFI);
    MTSSerial* serial = new MTSSerial(PTD3, PTD2, 256, 256);
    serial->baud(9600);
    Wifi::getInstance()->init(serial);
    
    code = Wifi::getInstance()->setNetwork("your wireless network" /* SSID of wireless */, Wifi::WPA2 /* security type of wireless */, "your wireless network password" /* password for wireless */);
    if(code == SUCCESS) {
        printf("Success!\r\n");
    } else {
        printf("Error during network setup [%d]\r\n", (int)code);
    }
    code = Wifi::getInstance()->setDeviceIP();
    if(code == SUCCESS) {
        printf("Success!\r\n");
    } else {
        printf("Error during IP setup [%d]\r\n", (int)code);
    }
#endif
    
    printf("Establishing Connection\r\n");
#if CELL_SHIELD
    if(Cellular::getInstance()->connect()) {
#else
    if(Wifi::getInstance()->connect()) {
#endif
        printf("Success!\r\n");
    } else {
        printf("Error during connection.  Aborting.\r\n");
        return;
    }
       
#if CELL_SHIELD
    if(Cellular::getInstance()->open(TEST_SERVER, TEST_PORT, IPStack::TCP)) {
#else
    if(Wifi::getInstance()->open(TEST_SERVER, TEST_PORT, IPStack::TCP)) {
#endif
        printf("Success!\r\n");   
    } else {
        printf("Error during TCP socket open [%s:%d].  Aborting.\r\n", TEST_SERVER.c_str(), TEST_PORT);
        return;
    }
    
    int count = 0;
    while(testTcpSocketEchoLoop()) {
        count++;  
        printf("Successful Echos: [%d]\r\n", count);  
    }
        
    printf("Closing socket\r\n");
#if CELL_SHIELD
    Cellular::getInstance()->close();
#else
    Wifi::getInstance()->close();
#endif
    
    wait(10);
    
    printf("Disconnecting\r\n");
#if CELL_SHIELD
    Cellular::getInstance()->disconnect();   
#else
    Wifi::getInstance()->disconnect();
#endif  
}

bool testTcpSocketEchoLoop() {
    using namespace mts;
    const char buffer[] = "*ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*";
    
    /*//Big Buffer
    const char buffer[] = "1ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "2ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "3ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "4ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "5ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "6ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "7ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "8ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "9ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*"
                          "0ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz1234567890*";
    */
                          
    const int size = sizeof(buffer); 
    char echoData[size];
    
    printf("Sending buffer\r\n");
#if CELL_SHIELD
    int bytesWritten = Cellular::getInstance()->write(buffer, size, 10000);
#else
    int bytesWritten = Wifi::getInstance()->write(buffer, size, 10000);
#endif
    if(bytesWritten == size) {
        printf("Successfully sent buffer\r\n");
    } else {
        printf("Failed to send buffer.  Closing socket and aborting.\r\n");
#if CELL_SHIELD
        Cellular::getInstance()->close();
#else
        Wifi::getInstance()->close();
#endif
        return false;
    }
    
    printf("Receiving echo (timeout = 15 seconds)\r\n");
    Timer tmr;
    int bytesRead = 0;
    tmr.start();
    do {
#if CELL_SHIELD
        int status = Cellular::getInstance()->read(&echoData[bytesRead], size - bytesRead, 1000);
#else
        int status = Wifi::getInstance()->read(&echoData[bytesRead], size - bytesRead, 1000);
#endif
        if(status != -1) {
            bytesRead += status;
        } else {
            printf("Error reading from socket.  Closing socket and aborting.\r\n");
#if CELL_SHIELD
            Cellular::getInstance()->close();
#else
            Wifi::getInstance()->close();
#endif
            return false;
        }
        printf("Total bytes read %d\r\n", bytesRead);
    } while (tmr.read_ms() <= 15000 && bytesRead < size);


    //Safely Cap at Max Size
    echoData[size - 1] = '\0';
    printf("Comparing Buffers\r\n");
    printf("SENT [%d]: [%s]\r\n", size, buffer);
    printf("RECV [%d]: [%s]\r\n", bytesRead, echoData);
    
    for(int i = 0; i < size - 1; i++) {
        if(buffer[i] != echoData[i]) {
            printf("Buffers do not match at index %d\r\n", i);
            return false;   
        }   
    }   
    return true;
}

#endif

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

#ifndef _TEST_TCP_SOCKET_H_
#define _TEST_TCP_SOCKET_H_

// 0 for shield board with wifi
// 1 for shield board with cellular
#define CELL_SHIELD 0

/* test TCP socket communication
 * will keep talking to server until data doesn't match expected */

using namespace mts;
const char PATTERN_LINE1[] = "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|";
const char PATTERN[] =  "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}|\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}/\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}-\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}\\\r\n"
                        "abcdefghijklmnopqrstuvwzyzABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$%^&*()[]{}*";
                        
const char MENU[] =     "1       send ascii pattern until keypress\r\n"
                        "2       send ascii pattern (numbered)\r\n"
                        "3       send pattern and close socket\r\n"
                        "4       send [ETX] and wait for keypress\r\n"
                        "5       send [DLE] and wait for keypress\r\n"
                        "6       send all hex values (00-FF)\r\n"
                        "q       quit\r\n"
                        ">:\r\n";
                        
const char WELCOME[] =  "Connected to: TCP test server";

bool testTcpSocketIteration();

void testTcpSocket() {
    Code code;
    /* this test is set up to interact with a server listening at the following address and port */
    const int TEST_PORT = 7000;
    const std::string TEST_SERVER("204.26.122.5";
    
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
        printf("Error during connection\r\n");
    }
       
    printf("Opening a TCP Socket\r\n");
#if CELL_SHIELD
    if(Cellular::getInstance()->open(TEST_SERVER, TEST_PORT, IPStack::TCP)) {
#else
    if(Wifi::getInstance()->open(TEST_SERVER, TEST_PORT, IPStack::TCP)) {
#endif
        printf("Success!\r\n");   
    } else {
        printf("Error during TCP socket open [%s:%d]\r\n", TEST_SERVER.c_str(), TEST_PORT);
    }
    
    //Find Welcome Message and Menu
    
    int count = 0;
    while(testTcpSocketIteration()) {
        count++;
        printf("Successful Iterations: %d\r\n", count);
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

bool testTcpSocketIteration() {
    Timer tmr;
    int bytesRead = 0;
    const int bufferSize = 1024;
    char buffer[bufferSize] = { 0 };
    std::string result;
    
    printf("Receiving Data\r\n");
    tmr.start();
    do {
#if CELL_SHIELD
        int size = Cellular::getInstance()->read(buffer, bufferSize, 1000);
#else
        int size = Wifi::getInstance()->read(buffer, bufferSize, 1000);
#endif
        if(size != -1) {
            result.append(buffer, size);
        } else {
            printf("Error reading from socket\r\n");
            return false;
        }
        printf("Total bytes read %d\r\n", result.size());
    } while (tmr.read() <= 15 && bytesRead < bufferSize);
   
    printf("READ: [%d] [%s]\r\n", bytesRead, result.c_str());
    
    size_t pos = result.find(PATTERN_LINE1);
    if(pos != std::string::npos) {
        //compare buffers
        int patternSize = sizeof(PATTERN) - 1;
        const char* ptr = &result.data()[pos];
        bool match = true;
        for(int i = 0; i < patternSize; i++) {
            if(PATTERN[i] != ptr[i]) {
                printf("1ST PATTERN DOESN'T MATCH AT [%d]\r\n", i);
                printf("PATTERN [%02X]  BUFFER [%02X]\r\n", PATTERN[i], ptr[i]);
                match = false;
                break;   
            }
        }
        if(match) {
            printf("FOUND 1ST PATTERN\r\n");   
        }
        
        pos = result.find(PATTERN_LINE1, pos + patternSize);
        if(pos != std::string::npos) {
            //compare buffers
            ptr = &result.data()[pos];
            match = true;
            for(int i = 0; i < patternSize; i++) {
                if(PATTERN[i] != ptr[i]) {
                    printf("2ND PATTERN DOESN'T MATCH AT [%d]\r\n", i);
                    printf("PATTERN [%02X]  BUFFER [%02X]\r\n", PATTERN[i], ptr[i]);
                    match = false;
                    break;   
                }
            }
            if(match) {
                printf("FOUND 2ND PATTERN\r\n");   
            }
        }
    }
    
    result.clear();
    
    printf("Writing to socket: 2\r\n");
#if CELL_SHIELD
    if(Cellular::getInstance()->write("2\r\n", 3, 10000) == 3) {
#else
    if(Wifi::getInstance()->write("2\r\n", 3, 10000) == 3) {
#endif
        printf("Successfully wrote '2'\r\n");
    } else {
        printf("Failed to write '2'\r\n");   
        return false;
    }
    printf("Expecting 'how many ? >:\r\n");
#if CELL_SHIELD
    bytesRead = Cellular::getInstance()->read(buffer, bufferSize, 10000);
#else
    bytesRead = Wifi::getInstance()->read(buffer, bufferSize, 10000);
#endif
    if(bytesRead != -1) {
        result.append(buffer, bytesRead);
        printf("READ: [%d] [%s]\r\n", bytesRead, result.c_str());
        if(result.find("how many") != std::string::npos) {
            printf("Successfully found 'how many'\r\n");   
            printf("Writing to socket: 2\r\n");
#if CELL_SHIELD
            if(Cellular::getInstance()->write("2\r\n", 3, 10000) == 3) {
#else
            if(Wifi::getInstance()->write("2\r\n", 3, 10000) == 3) {
#endif
                printf("Successfully wrote '2'\r\n");
            } else {
                printf("Failed to write '2'\r\n");   
                return false;
            }
        } else {
            printf("Missing second option to menu item 2\r\n");
        }
    } else {
        printf("Error reading from socket\r\n");
        return false;
    }
    
    return true;
}

#endif

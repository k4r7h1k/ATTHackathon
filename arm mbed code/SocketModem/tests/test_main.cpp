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

#include "mbed.h"
#include "include_me.h"

// uncomment only the header corresponding to the test you want to run
//#include "test_ping.h"
//#include "test_SMS.h"
//#include "test_TCP_Socket.h"
//#include "test_TCP_Socket_Echo.h"
//#include "test_MTS_Circular_Buffer.h"


//int main() {
    // uncomment only one test at a time
    
    // PING TEST
    //testPing();

    /*
    // SMS TEST
    for (int i = 30; i >= 0; i = i - 2) {
        wait(2);
        printf("Waiting %d seconds...\n\r", i);
    }  
    Transport::setTransport(Transport::CELLULAR);
    MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
    serial->baud(115200);
    Cellular* cell = Cellular::getInstance();
    cell->init(serial);
    while (cell->getRegistration() != Cellular::REGISTERED);
    while (cell->setApn("wap.cingular") != SUCCESS);
    
    sendSms();
    while (true) {
        receiveSms();
        wait(15);
    }
    */
    
    // TCP SOCKET TEST
    //testTcpSocket();
    
    // TCP SOCKET ECHO TEST
    //testTcpSocketEcho();
    
    // CIRCULAR BUFFER TEST
    //testMTSCircularBuffer();
//}

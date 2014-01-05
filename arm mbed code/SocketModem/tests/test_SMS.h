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

#ifndef _TEST_SMS_H_
#define _TEST_SMS_H_

using namespace mts;

void sendSms() {
    Code code;
    std::string sMsg("Hello from Multi-Tech MBED!");
    std::string sPhoneNum( /* your 10-digit phone number prepended with a 1, e.g. 12228675309 */);
    
    printf("Sending message [%s] to [%s]\r\n", sMsg.c_str(), sPhoneNum.c_str());
    code = Cellular::getInstance()->sendSMS(sPhoneNum, sMsg);
    
    if(code != SUCCESS) {
        printf("Error during SMS send [%d]\r\n", (int)code);
    } else {
        printf("Success!\r\n");
    }
}

void receiveSms() {
    printf("Checking Received Messages\r\n");
    std::vector<Cellular::Sms> vSms = Cellular::getInstance()->getReceivedSms();
    printf("\r\n");
    for(unsigned int i = 0; i < vSms.size(); i++) {
        printf("[%d][%s][%s][%s]\r\n", i, vSms[i].timestamp.c_str(), 
                vSms[i].phoneNumber.c_str(), vSms[i].message.c_str());
    }
}

#endif


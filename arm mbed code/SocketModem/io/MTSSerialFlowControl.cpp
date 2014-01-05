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

#include "MTSSerialFlowControl.h"

using namespace mts;

MTSSerialFlowControl::MTSSerialFlowControl(PinName TXD, PinName RXD, PinName RTS, PinName CTS, int txBufSize, int rxBufSize)
    : MTSSerial(TXD, RXD, txBufSize, rxBufSize)
    , rxReadyFlag(false)
    , rts(RTS)
    , cts(CTS)
{
    notifyStartSending();

    highThreshold = MAX(rxBufSize - 10, rxBufSize * 0.85);
    lowThreshold = rxBufSize * 0.3;

    rxBuffer.attach(this, &MTSSerialFlowControl::notifyStartSending, lowThreshold, Vars::LESS);
}

MTSSerialFlowControl::~MTSSerialFlowControl()
{

}

void MTSSerialFlowControl::rxClear()
{
    MTSBufferedIO::rxClear();
    notifyStartSending();
}

void MTSSerialFlowControl::notifyStartSending()
{
    if(!rxReadyFlag) {
        rts.write(0);
        rxReadyFlag = true;
        //printf("RTS LOW: READY - RX[%d/%d]\r\n", rxBuffer.size(), rxBuffer.capacity());
    }
}

void MTSSerialFlowControl::notifyStopSending()
{
    if(rxReadyFlag) {
        rts.write(1);
        rxReadyFlag = false;
        //printf("RTS HIGH: NOT-READY - RX[%d/%d]\r\n", rxBuffer.size(), rxBuffer.capacity());
    }
}

void MTSSerialFlowControl::handleRead()
{
    while (serial.readable()) {
        char byte = serial.getc();
        if(rxBuffer.write(byte) != 1) {
            rts.write(1);
            rxReadyFlag = false;
            printf("[ERROR] Serial Rx Byte Dropped [%c][0x%02X]\r\n", byte, byte);
            if(byte == 0xFF) {
                // hack so we dont hang - fix later
                puts("[ERR] Comm errors, must reboot");
                fflush(stdout);
                NVIC_SystemReset();
            }
            return;
        }
        if (rxBuffer.size() > highThreshold) {
            notifyStopSending();
        }
    }
}

void MTSSerialFlowControl::handleWrite()
{
    while(txBuffer.size() != 0) {
        if (serial.writeable() && cts.read() == 0) {
            char byte;
            if(txBuffer.read(byte) == 1) {
                serial.putc(byte);
            }
        } else {
            return;
        }
    }
}


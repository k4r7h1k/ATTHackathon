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

#include "MTSSerial.h"

using namespace mts;

MTSSerial::MTSSerial(PinName TXD, PinName RXD, int txBufferSize, int rxBufferSize)
    : MTSBufferedIO(txBufferSize, rxBufferSize)
    , serial(TXD,RXD)
{
    serial.attach(this, &MTSSerial::handleRead, Serial::RxIrq);
    //serial.attach(this, &MTSSerial::handleWrite, Serial::TxIrq);
}

MTSSerial::~MTSSerial()
{

}

void MTSSerial::baud(int baudrate)
{
    serial.baud(baudrate);
}

void MTSSerial::format(int bits, SerialBase::Parity parity, int stop_bits)
{
    serial.format(bits, parity, stop_bits);
}

void MTSSerial::handleRead()
{
    while (serial.readable()) {
        char byte = serial.getc();
        if(rxBuffer.write(byte) != 1) {
            printf("[ERROR] Serial Rx Byte Dropped [%c][0x%02X]\r\n", byte, byte);
            if(byte == 0xFF) {
                // hack so we dont hang - fix later
                puts("[ERR] Comm errors, must reboot");
                fflush(stdout);
                NVIC_SystemReset();
            }
            return;
        }
    }
}

// Currently uses Non-Irq based blocking write calls
void MTSSerial::handleWrite()
{
    while(txBuffer.size() != 0) {
        if (serial.writeable()) {
            char byte;
            if(txBuffer.read(byte) == 1) {
                serial.putc(byte);
            }
        } else {
            return;
        }
    }
}



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

#include "MTSBufferedIO.h"

using namespace mts;

MTSBufferedIO::MTSBufferedIO(int txBufferSize, int rxBufferSize)
: txBuffer(txBufferSize)
, rxBuffer(rxBufferSize)
{

}

MTSBufferedIO::~MTSBufferedIO()
{

}

int MTSBufferedIO::write(const char* data, int length, unsigned int timeoutMillis) 
{
    //Writes until empty or timeout is reached (different implementation planned once tx isr is working)
    int bytesWritten = 0;
    Timer tmr;
    tmr.start();
    length = MAX(0,length);
    do {
        int bytesWrittenSwBuffer = txBuffer.write(&data[bytesWritten], length - bytesWritten);
        if(bytesWrittenSwBuffer > 0) {
            handleWrite();
            int bytesRemainingSwBuffer = txBuffer.size();
            txBuffer.clear();
            bytesWritten += (bytesWrittenSwBuffer - bytesRemainingSwBuffer);
        }
    } while(tmr.read_ms() <= timeoutMillis && bytesWritten < length);
    return bytesWritten;
}

int MTSBufferedIO::write(const char* data, int length)
{   
    //Blocks until all bytes are written (different implementation planned once tx isr is working)
    int bytesWritten = 0;
    length = MAX(0,length);
    do {
        int bytesWrittenSwBuffer = txBuffer.write(&data[bytesWritten], length - bytesWritten);
        handleWrite();
        int bytesRemainingSwBuffer = txBuffer.size();
        txBuffer.clear();
        bytesWritten += bytesWrittenSwBuffer - bytesRemainingSwBuffer;
    } while(bytesWritten < length);
    return length;
}

int MTSBufferedIO::write(char data, unsigned int timeoutMillis) 
{
    return write(&data, 1, timeoutMillis);
}

int MTSBufferedIO::write(char data)
{
    return write(&data, 1);
}

int MTSBufferedIO::writeable() {
    return txBuffer.remaining();   
}

int MTSBufferedIO::read(char* data, int length, unsigned int timeoutMillis) 
{
    int bytesRead = 0;
    Timer tmr;
    tmr.start();
    length = MAX(0,length);
    do {
        bytesRead += rxBuffer.read(&data[bytesRead], length - bytesRead);
    } while(tmr.read_ms() <= timeoutMillis && bytesRead < length);
    return bytesRead;
}

int MTSBufferedIO::read(char* data, int length)
{
    int bytesRead = 0;
    length = MAX(0,length);
    while(bytesRead < length) {
        bytesRead += rxBuffer.read(&data[bytesRead], length - bytesRead);
    }
    return length;
}

int MTSBufferedIO::read(char& data, unsigned int timeoutMillis) 
{
    return read(&data, 1, timeoutMillis);
}

int MTSBufferedIO::read(char& data)
{
    return rxBuffer.read(&data, 1);
}

int MTSBufferedIO::readable() {
    return rxBuffer.size();   
}

bool MTSBufferedIO::txEmpty()
{
    return txBuffer.isEmpty();
}

bool MTSBufferedIO::rxEmpty()
{
    return rxBuffer.isEmpty();
}

bool MTSBufferedIO::txFull()
{
    return txBuffer.isFull();
}

bool MTSBufferedIO::rxFull()
{
    return rxBuffer.isFull();
}

void MTSBufferedIO::txClear()
{
    txBuffer.clear();
}

void MTSBufferedIO::rxClear()
{
    rxBuffer.clear();
}

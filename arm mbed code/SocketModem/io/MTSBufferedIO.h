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

#ifndef MTSBUFFEREDIO_H
#define MTSBUFFEREDIO_H

#include "mbed.h"
#include "MTSCircularBuffer.h"

namespace mts {

/** This is an abstract class for lightweight buffered io to an underlying
* data interface. Specifically the inheriting class will need to override
* both the handleRead and handleWrite methods which transfer data between
* the class's internal read and write buffers and the physical communications
* link or its HW buffers.
*/

class MTSBufferedIO
{
public:
    /** Creates a new BufferedIO object with the passed in static buffer sizes.
    * Note that because this class is abstract you cannot construct it directly.
    * Instead, please construct one of its derived classes like MTSSerial or
    * MTSSerialFlowControl.
    *
    * @param txBufferSize the size of the Tx or write buffer in bytes. The default is
    * 128 bytes.
    * @param rxBufferSize the size of the Rx or read buffer in bytes. The default is
    * 128 bytes.
    */
    MTSBufferedIO(int txBufferSize = 128, int rxBufferSize = 128);

    /** Destructs an MTSBufferedIO object and frees all related resources, including
    * internal buffers.
    */
    ~MTSBufferedIO();

    /** This method enables bulk writes to the Tx or write buffer. If more data
    * is requested to be written then space available the method writes
    * as much data as possible within the timeout period and returns the actual amount written.
    *
    * @param data the byte array to be written.
    * @param length the length of data to be written from the data parameter.
    * @timeoutMillis amount of time in milliseconds to complete operation.
    * @returns the number of bytes written to the buffer, which is 0 if
    * the buffer is full.
    */
    int write(const char* data, int length, unsigned int timeoutMillis);
    
    /** This method enables bulk writes to the Tx or write buffer. If more data
    * is requested to be written then space available the method writes
    * as much data as possible and returns the actual amount written.
    *
    * @param data the byte array to be written.
    * @param length the length of data to be written from the data parameter.
    * @returns the number of bytes written to the buffer, which is 0 if
    * the buffer is full.
    */
    int write(const char* data, int length);

    /** This method attempts to write a single byte to the tx buffer
    * within the timeout period.
    *
    * @param data the byte to be written as a char.
    * @timeoutMillis amount of time in milliseconds to complete operation.
    * @returns 1 if the byte was written or 0 if the buffer was full.
    */
    int write(char data, unsigned int timeoutMillis);
    
    /** This method writes a signle byte as a char to the Tx or write buffer.
    *
    * @param data the byte to be written as a char.
    * @returns 1 if the byte was written or 0 if the buffer was full.
    */
    int write(char data);

    /** This method is used to get the space available to write bytes to the Tx buffer.
    *
    * @returns the number of bytes that can be written, 0 if the buffer is full.
    */
    int writeable();

    /** This method enables bulk reads from the Rx or read buffer.  If more data is
    * requested then available it simply returns all remaining data within the
    * buffer.
    *
    * @param data the buffer where data read will be added to.
    * @param length the amount of data in bytes to be read into the buffer.
    * @timeoutMillis amount of time to complete operation.
    * @returns the total number of bytes that were read.
    */
    int read(char* data, int length, unsigned int timeoutMillis);
    
    /** This method enables bulk reads from the Rx or read buffer.  If more data is
    * requested then available it simply returns all remaining data within the
    * buffer.
    *
    * @param data the buffer where data read will be added to.
    * @param length the amount of data in bytes to be read into the buffer.
    * @returns the total number of bytes that were read.
    */
    int read(char* data, int length);

    /** This method reads a single byte from the Rx or read buffer.
    *
    * @param data char where the read byte will be stored.
    * @timeoutMillis amount of time to complete operation.
    * @returns 1 if byte is read or 0 if no byte is available.
    */
    int read(char& data, unsigned int timeoutMillis);
    
    /** This method reads a single byte from the Rx or read buffer.
    *
    * @param data char where the read byte will be stored.
    * @returns 1 if byte is read or 0 if no byte is available.
    */
    int read(char& data);

    /** This method is used to get the number of bytes available to read from
    * the Rx or read buffer.
    *
    * @returns the number of bytes available, 0 if there are no bytes to read.
    */
    int readable();

    /** This method determines if the Tx or write buffer is empty.
    *
    * @returns true if empty, otherwise false.
    */
    bool txEmpty();

    /** This method determines if the Rx or read buffer is empty.
    *
    * @returns true if empty, otherwise false.
    */
    bool rxEmpty();

    /** This method determines if the Tx or write buffer is full.
    *
    * @returns true if full, otherwise false.
    */
    bool txFull();

    /** This method determines if the Rx or read buffer is full.
    *
    * @returns true if full, otherwise false.
    */
    bool rxFull();

    /** This method clears all the data from the internal Tx or write buffer.
    */
    virtual void txClear();

    /** This method clears all the data from the internal Rx or read buffer.
    */
    virtual void rxClear();

    /** This abstract method should be used by the deriving class to transfer
    * data from the internal write buffer (txBuffer) to the physical interface.
    * Note that this function is called everytime new data is written to the
    * txBuffer though one of the write calls.
    */
    virtual void handleWrite() = 0;

    /** This abstract method should be used by the deriving class to transfer
    * data from the physical interface ot the internal read buffer (rxBuffer).
    * Note that this function is never called in this class and typically should
    * be called as part of a receive data interrupt routine.
    */
    virtual void handleRead() = 0;

protected:
    MTSCircularBuffer txBuffer; // Internal write or transmit circular buffer
    MTSCircularBuffer rxBuffer; // Internal read or receieve circular buffer
};

}

#endif /* MTSBUFFEREDIO_H */

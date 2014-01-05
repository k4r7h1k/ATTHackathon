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

#ifndef MTSSERIALFLOWCONTROL_H
#define MTSSERIALFLOWCONTROL_H

#include "mbed.h"
#include "MTSSerial.h"


namespace mts
{

/** This class derives from MTSBufferedIO/MTSSerial and provides a buffered wrapper to the
* standard mbed Serial class along with generic RTS/CTS HW flow control. Since it
* depends only on the mbed Serial, DigitalOut and InterruptIn classes for accessing
* the serial data, this class is inherently portable accross different mbed platforms
* and provides HW flow control even when not natively supported by the processors
* serial port. If HW flow control is not needed, use MTSSerial instead. It should also
* be noted that the RTS/CTS functionality in this class is implemented as a DTE device.
*/
class MTSSerialFlowControl : public MTSSerial
{
public:
    /** Creates a new MTSSerialFlowControl object that can be used to talk to an mbed serial
    * port through internal SW buffers. Note that this class also adds the ability to use
    * RTS/CTS HW Flow Conrtol through and standard mbed DigitalIn and DigitalOut pins.
    * The RTS and CTS functionality assumes this is a DTE device.
    *
    * @param TXD the transmit data pin on the desired mbed serial interface.
    * @param RXD the receive data pin on the desired mbed serial interface.
    * @param RTS the DigitalOut pin that RTS will be attached to. (DTE)
    * @param CTS the DigitalIn pin that CTS will be attached to. (DTE)
    * @param txBufferSize the size in bytes of the internal SW transmit buffer. The
    * default is 64 bytes.
    * @param rxBufferSize the size in bytes of the internal SW receive buffer. The
    * default is 64 bytes.
    * @param name an optional name for the serial port. The default is blank.
    */
    MTSSerialFlowControl(PinName TXD, PinName RXD, PinName RTS, PinName CTS, int txBufSize = 64, int rxBufSize = 64);

    /** Destructs an MTSSerialFlowControl object and frees all related resources,
    * including internal buffers.
    */
    ~MTSSerialFlowControl();
    
    /** This method clears all the data from the internal Rx or read buffer.
    */
    virtual void rxClear();

private:
    void notifyStartSending(); // Used to set cts start signal
    void notifyStopSending(); // Used to set cts stop signal
    
    //This device acts as a DTE
    bool rxReadyFlag;   //Tracks state change for rts signaling
    DigitalOut rts; // Used to tell DCE to send or not send data
    DigitalIn cts; // Used to check if DCE is ready for data
    int highThreshold; // High water mark for setting cts to stop
    int lowThreshold; // Low water mark for setting cts to start

    virtual void handleRead(); // Method for handling data to be read
    virtual void handleWrite(); // Method for handling data to be written
};

}

#endif /* MTSSERIALFLOWCONTROL */

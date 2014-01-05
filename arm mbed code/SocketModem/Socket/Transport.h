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

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include "mbed.h"
#include "IPStack.h"

using namespace mts;

/** This class has been added to the standard mbed Socket library enabling people
* to use the Socket library interfaces for different transports that have
* their own internal IP-Stack. Use this class prior to instantiating any of the
* other classes in this folder to determine the underlying transport that will
* be used by them. It is important to know that the transport classes themsleves
* like Cellular or WiFi, must be properly initialized and connected before any
* of the Socket package classes can be used or even instantiated.
*/
class Transport
{
public:
    ///An enumeration that holds the supported Transport Types.
    enum TransportType {
        CELLULAR, WIFI, NONE
    };
    
    /** This method allows you to set the transport to be used when creating other 
    * objects from the Socket folder like TCPSocketConnection and UDPSocket.  
    *
    * @param type the type of underlying transport to be used. The default is NONE.
    */
    static void setTransport(TransportType type);
    
    /** This method is used within the Socket class to get the appropraite transport
    * as an IPStack object.  In general you do not need to call this directly, but
    * simply use the other classes in this folder. 
    *
    * @returns a pointer to an object that implements IPStack.
    */
    static IPStack* getInstance();
    
private:
    static Transport::TransportType _type; // Member variable that holds the desired transport
};

#endif /* TRANSPORT_H */
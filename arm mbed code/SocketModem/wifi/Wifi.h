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

#ifndef WIFI_H
#define WIFI_H

#include "IPStack.h"
#include "MTSBufferedIO.h"
#include "mbed.h"
#include <string>
#include <vector>

using namespace mts;

/** This is a class for communicating with a Roving Networks RN-171 Wifi module. This
* module comes in a variety of form factors including the Multi-Tech SocketShield.
* This class supports two main types of WiFi module interactions including:
* configuration and status command processing and TCP Socket
* data connections. It should be noted that while a data connection is open the module
* must be put in command mode before commands can be sent. This is handled within the class
* automatically for all native commands. This class also inherits from IPStack
* providing a common set of commands for communication devices that have an onboard
* IP Stack. It is also integrated with the standard mbed Sockets package and can therefore
* be used seamlessly with clients and services built on top of this interface already within
* the mbed library.
*
* All of the following examples use the Pin Names for the Freedom KL46Z board coupled with
* the SocketModem Shield Arduino compatible board. Please chage Pin Names accordingly to
* match your hardware configuration. The default baud rate for the WiFi module is 9600 bps.
*
* The following example shows how to connect to a WiFi netork and perform a basic ping test:
* @code
* #include "mbed.h"
* #include "MTSSerial.h"
* #include "Wifi.h"
* using namespace mts;
*
* int main()
* {
*   std::string ssid = "Your SSID goes here";
*   std::string securityKey = "Your secuirty key goes here";
*   Wifi::SecurityType securityType = Wifi::WPA2;
*
*   //Wait for wifi module to boot up
*   for (int i = 10; i >= 0; i = i - 2) {
*       wait(2);
*       printf("Waiting %d seconds...\n\r", i);
*   }
*
*   //Setup serial interface to WiFi module
*   MTSSerial* serial = new MTSSerial(PTD3, PTD2, 256, 256);
*   serial->baud(9600);
*
*   //Setup Wifi class
*   Wifi* wifi = Wifi::getInstance();
*   printf("Init: %s\n\r", wifi->init(serial) ? "SUCCESS" : "FAILURE");
*
*   //Setup and check connection
*   printf("Set Network: %s\n\r", getCodeNames(wifi->setNetwork(ssid, securityType, securityKey)).c_str());
*   printf("Set DHCP: %s\n\r", getCodeNames(wifi->setDeviceIP("DHCP")).c_str());
*   printf("Connect: %s\n\r", wifi->connect() ? "Success" : "Failure");
*   printf("Is Connected: %s\n\r", wifi->isConnected() ? "True" : "False");
*   printf("Ping Server: %s\n\r", wifi->ping("8.8.8.8") ? "Success" : "Failed");
*
*   //Disconnect from network
*   printf("Disconnecting...\n\r");
*   wifi->disconnect();
*   printf("Is Connected: %s\n\r", wifi->isConnected() ? "True" : "False");
*
*   printf("End Program\n\r");
* }
* @endcode
*/
class Wifi : public IPStack
{
public:
    ///An enumeration for all the supported WiFi security types.
    enum SecurityType {
        NONE, WEP64, WEP128, WPA, WPA2
    };

    /** Destructs a Wifi object and frees all related resources.
    */
    ~Wifi();

    /** This static function is used to create or get a reference to a
    * Wifi object. Wifi uses the singleton pattern, which means
    * that you can only have one existing at a time. The first time you
    * call getInstance this method creates a new uninitialized Wifi
    * object and returns it. All future calls to this method will return
    * a reference to the instance created during the first call. Note that
    * you must call init on the returned instance before mnaking any other
    * calls. If using this class's bindings to any of the Socket package
    * classes like TCPSocketConnection, you must call this method and the
    * init method on the returned object first, before even creating the
    * other objects.
    *
    * @returns a reference to the single Wifi obect that has been created.
    */
    static Wifi* getInstance();

    /** This method initializes the object with the underlying Wifi module
    * interface to use. Note that this function MUST be called before
    * any other calls will function correctly on a Wifi object. Also
    * note that MTSBufferedIO is abstract, so you must use one of
    * its inherited classes like MTSSerial or MTSSerialFlowControl.
    *
    * @param io the buffered io interface that is attached to the wifi
    * radio module.
    * @returns true if the init was successful, otherwise false.
    */
    bool init(MTSBufferedIO* io);

    /** This method establishes a network connection on the Wif radio module.
    * Note that before calling you NEED to first set the network information
    * including WiFi SSID and optional security key using the setNetwork
    * method.
    *
    * @returns true if the connection was successfully established, otherwise
    * false on an error.
    */
    virtual bool connect();

    /** This method is used to stop a previously established Wifi network connection.
    */
    virtual void disconnect();

    /** This method is used to check if the radio currently has a Wifi network
    * connection established.
    *
    * @returns true if a network connection exists, otherwise false.
    */
    virtual bool isConnected();

    // TCP and UDP Socket related commands
    // For behavior of the following methods refer to IPStack.h documentation
    virtual bool bind(unsigned int port);
    virtual bool open(const std::string& address, unsigned int port, Mode mode);
    virtual bool isOpen();
    virtual bool close();
    virtual int read(char* data, int max, int timeout = -1);
    virtual int write(const char* data, int length, int timeout = -1);
    virtual unsigned int readable();
    virtual unsigned int writeable();

    /** This method performs a soft reboot of the device by issuing the
    * reboot command. If the module is not able to respond to commands
    * this will not work. This method also waits 10 seconds for the
    * module to perform the reset and return.
    */
    virtual void reset();

    /** A method for sending a generic text command to the radio. Note that you cannot
    * send commands and have a socket connection at the same time, unless you first
    * switch to command mode.
    *
    * @param command the command to send to the WiFi module without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param response the text string to look for and to return immediately after finding.
    * The default is to look for no specific response.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).  Does not append any character if esc == 0.
    * @returns all data received from the radio after the command as a string.
    */
    std::string sendCommand(std::string command, int timeoutMillis, std::string response = "", char esc = CR);

    /** A method for sending a basic command to the radio. A basic text command is
    * one that simply has a response of either AOK or ERR without any other information.
    * Note that you cannot send commands and have a tcp connection at the same time
    * unless you first switch to command mode.
    *
    * @param command the command to send to the WiFi module without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).
    * @returns the standard Code enumeration.
    */
    Code sendBasicCommand(std::string command, int timeoutMillis, char esc = CR);

    /** This method is used to set the network information details. This method must be
    * called before connect, which establishes the WiFi network connection.
    *
    * @param ssid the SSID for the network you want to attached to.
    * @param type the type of security used on the network. The default is NONE.
    * @param key the security key for the network. The default is no key.
    */
    Code setNetwork(const std::string& ssid, SecurityType type = NONE, const std::string& key = "");

    /** This method is used to set the IP address or puts the module in DHCP mode.
    *
    * @param address the IP address you want to use in the form of xxx.xxx.xxx.xxx or DHCP
    * if you want to use DHCP. The default is DHCP.
    * @returns the standard Code enumeration.
    */
    Code setDeviceIP(std::string address = "DHCP");

    /** This method is used to get the IP address of the device, which can be
    * set either statically or via DHCP after connecting to a network.
    *
    * @returns the devices IP address.
    */
    std::string getDeviceIP();

    /** This method is used to set the DNS which enables the use of URLs instead
    * of IP addresses when making a socket connection.
    *
    * @param the DNS server address as a string in form xxx.xxx.xxx.xxx.
    * @returns the standard AT Code enumeration.
    */
    Code setDNS(const std::string& dnsName);

    /** A method for getting the signal strength of the Wifi module. This method allows
    * you to get the signal strength in dBm. If you get a result of 99 the signal strength
    * is not known or there was an error in reading it. Note that you cannot read the signal
    * strength unless you are already attached to a Wifi network.
    *
    * @returns an integer representing the signal strength in dBm.
    */
    int getSignalStrength();

    /** This method is used test network connectivity by pinging a server.
    *
    * @param address the address of the server in format xxx.xxx.xxx.xxx.
    * @returns true if the ping was successful, otherwise false.
    */
    bool ping(const std::string& address = "8.8.8.8");

    /** This method is used to set whether the device is in command mode or data mode.
    * In command mode you are able to send configuration and status commands while
    * data mode is used for sending data when you have an open socket connection.
    * Note that for all other methods in this class the change is handled automatically.
    * Only use this methodif you want to send your own commands that are not already
    * supported and need to make sure that you are in command mode.
    *
    * @param on if true sets to command mode, otherwise to data mode.
    * @returns true if the change was successful, otherwise false.
    */
    bool setCmdMode(bool on);

private:
    static Wifi* instance; //Static pointer to the single Cellular object.

    MTSBufferedIO* io; //IO interface obect that the radio is accessed through.

    bool wifiConnected; //Specifies if a Wifi network session is currently connected.
    std::string _ssid; //A string that holds the SSID for the Wifi module.

    Mode mode; //The current socket Mode.
    bool socketOpened; //Specifies if a Socket is presently opened.
    bool socketCloseable; //Specifies is a Socket can be closed.
    unsigned int local_port; //Holds the local port for socket connections.
    std::string local_address; //Holds the local address for socket connections.
    unsigned int host_port; //Holds the remote port for socket connections.
    std::string host_address; //Holds the remote address for socket connections.
    bool cmdOn; //Determines whether the device is in command mode or not

    Wifi(); //Private constructor, use the getInstance() method.
    Wifi(MTSBufferedIO* io); //Private constructor, use the getInstance() method.
    bool sortInterfaceMode(void); // module gets in wierd state without IO reset
    std::string getHostByName(std::string url); // Gets the IP address for a URL
};

#endif /* WIFI_H */
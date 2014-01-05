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

#ifndef CELLULAR_H
#define CELLULAR_H

#include "IPStack.h"
#include "MTSBufferedIO.h"
#include "mbed.h"
#include <string>
#include <vector>

namespace mts
{

/** This is a class for communicating with a Multi-Tech Systems SocketModem iCell. The
* SocketModem iCell is a family of carrier certified embedded cellular radio modules with
* a common hardware footprint and AT command set for built in IP-stack functionality.
* This class supports three main types of cellular radio interactions including:
* configuration and status AT command processing, SMS processing, and TCP Socket
* data connections. It should be noted that the radio can not process commands or
* SMS messages while having an open data connection at the same time. The concurrent
* capability may be added in a future release. This class also inherits from IPStack
* providing a common set of commands for communication devices that have an onboard
* IP Stack. It is also integrated with the standard mbed Sockets package and can therefore
* be used seamlessly with clients and services built on top of this interface already within
* the mbed library.
*
* All of the following examples use the Pin Names for the Freedom KL46Z board coupled with
* the SocketModem Shield Arduino compatible board. Please chage Pin Names accordingly to
* match your hardware configuration. It also assumes the use of RTS/CTS hardware handshaking
* using GPIOs. To disable this you will need to change settings on the radio module and
* and use the MTSSerial class instead of MTSSerialFlowControl. The default baud rate for the
* cellular radio is 115200 bps.
*
* The following set of example code demonstrates how to send and receive configuration and
* status AT commands with the radio, create a data connection and test it:
* @code
* #include "mbed.h"
* #include "Cellular.h"
* #include "MTSSerialFlowControl.h"
*
* using namespace mts;
*
* main() {
*   //Wait for radio to boot up
*   for (int i = 30; i >= 0; i = i - 5) {
*       wait(5);
*        printf("Waiting %d seconds...\n\r", i);
*   }
*
*   //Setup serial interface to radio
*   MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
*   serial->baud(115200);
*
*   //Setup Cellular class
*   Cellular* cellular = Cellular::getInstance();
*   cellular->init(serial, PTA4, PTC9); //DCD and DTR pins for KL46Z
*
*   //Run status and configuration commands
*   printf("\n\r////Start Status and Configuration Commands////\n\r");
*   printf("Command Test: %s\n\r", getCodeNames(cellular->test()).c_str());
*   printf("Signal Strength: %d\n\r", cellular->getSignalStrength());
*   printf("Registration State: %s\n\r", Cellular::getRegistrationNames(cellular->getRegistration()).c_str());
*   printf("Send Basic Command (AT): %s\n\r", getCodeNames(cellular->sendBasicCommand("AT", 1000)).c_str());
*   printf("Send Command (AT+CSQ): %s\n\r", cellular->sendCommand("AT+CSQ", 1000).c_str());
*
*   //Start Test
*   printf("\n\r////Start Network Connectivity Test////\n\r");
*   printf("Set APN: %s\n\r", getCodeNames(cellular->setApn("wap.cingular")).c_str()); //Use APN from service provider!!!
*
*   //Setup a data connection
*   printf("Attempting to Connect, this may take some time...\n\r");
*   while (!cellular->connect()) {
*       printf("Failed to connect... Trying again.\n\r");
*       wait(1);
*   }
*   printf("Connected to the Network!\n\r");
*
*   //Try pinging default server "8.8.8.8"
*   printf("Ping Valid: %s\n\r", cellular->ping() ? "true" : "false");
*
*   printf("End Program\n\r");
* }
* @endcode
*
* The following set of example code demonstrates how process SMS messages:
* @code
* #include "mbed.h"
* #include "Cellular.h"
* #include "MTSSerialFlowControl.h"
*
* using namespace mts;
*
* main() {
*   //Wait for radio to boot up
*   for (int i = 30; i >= 0; i = i - 5) {
*       wait(5);
*        printf("Waiting %d seconds...\n\r", i);
*   }
*
*   //Setup serial interface to radio
*   MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
*   serial->baud(115200);
*
*   //Setup Cellular class
*   Cellular* cellular = Cellular::getInstance();
*   cellular->init(serial, PTA4, PTC9); //DCD and DTR pins for KL46Z
*
*   //Start test
*   printf("AT Test: %s\n\r", getCodeNames(cellular->test()).c_str());
*
*   //Waiting for network registration
*   printf("Checking Network Registration, this may take some time...\n\r");
*   while (cellular->getRegistration() != Cellular::REGISTERED) {
*       printf("Still waiting... Checking again.\n\r");
*       wait(1);
*   }
*   printf("Connected to the Network!\n\r");
*
*   //Send SMS Message
*   Code code;
*   std::string sMsg("Hello from Multi-Tech MBED!");
*   std::string sPhoneNum("16128675309"); //Put your phone number here or leave Jenny's...
*
*   printf("Sending message [%s] to [%s]\r\n", sMsg.c_str(), sPhoneNum.c_str());
*   code = cellular->sendSMS(sPhoneNum, sMsg);
*
*   if(code != SUCCESS) {
*       printf("Error during SMS send [%s]\r\n", getCodeNames(code).c_str());
*   } else {
*       printf("Success!\r\n");
*   }
*
*   //Try and receive SMS messages
*   //To determine your radio's phone number send yourself an SMS and check the received #
*   printf("Checking Received Messages\r\n");
*   std::vector<Cellular::Sms> vSms = cellular->getReceivedSms();
*   printf("\r\n");
*   for(unsigned int i = 0; i < vSms.size(); i++) {
*       printf("[%d][%s][%s][%s]\r\n", i, vSms[i].timestamp.c_str(),
*               vSms[i].phoneNumber.c_str(), vSms[i].message.c_str());
*   }
*   printf("End Program\n\r");
* }
* @endcode
*
* The following set of example code demonstrates how to setup and use a TCP socket connection
* using the native commands from this class:
* @code
* #include "mbed.h"
* #include "Cellular.h"
* #include "MTSSerialFlowControl.h"
*
* using namespace mts;
*
* main() {
*   //Define connection parameters
*   Code code;
*   const int TEST_PORT = 5798;
*   const std::string TEST_SERVER("204.26.122.96");
*
*   //Wait for radio to boot up
*   for (int i = 30; i >= 0; i = i - 5) {
*       wait(5);
*        printf("Waiting %d seconds...\n\r", i);
*   }
*
*   //Setup serial interface to radio
*   MTSSerialFlowControl* serial = new MTSSerialFlowControl(PTD3, PTD2, PTA12, PTC8);
*   serial->baud(115200);
*
*   //Setup Cellular class
*   Cellular* cellular = Cellular::getInstance();
*   cellular->init(serial, PTA4, PTC9); //DCD and DTR pins for KL46Z
*
*   //Start test
*   printf("AT Test: %s\n\r", getCodeNames(cellular->test()).c_str());
*
*   printf("Setting APN\r\n");
*   code = cellular->setApn("wap.cingular"); // Use from your service provider!
*   if(code == SUCCESS) {
*       printf("Success!\r\n");
*   } else {
*       printf("Error during APN setup [%s]\r\n", getCodeNames(code).c_str());
*   }
*
*   //Setup a data connection
*   printf("Attempting to Connect, this may take some time...\n\r");
*   while (!cellular->connect()) {
*       printf("Failed to connect... Trying again.\n\r");
*       wait(1);
*   }
*   printf("Connected to the Network!\n\r");
*
*   printf("Opening a TCP Socket...\r\n");
*   if(cellular->open(TEST_SERVER, TEST_PORT, IPStack::TCP)) {
*       printf("Success!\r\n");
*   } else {
*       printf("Error during TCP socket open [%s:%d]\r\n", TEST_SERVER.c_str(), TEST_PORT);
*   }
*
*   char data[] = "My Test Echo Message";
*   int size = sizeof(data);
*   printf("WRITE: [%d] [%s]\r\n", size, data);
*   int bytesWritten = cellular->write(data, size, 10000);
*   if(bytesWritten == size) {
*       printf("Successfully wrote message!\r\n");
*   } else {
*       printf("Failed to write message!\r\n");
*   }
*
*   printf("Waiting 5 seconds\r\n");
*   wait(5);
*
*   printf("Reading from socket for 10 seconds\r\n");
*   char response[size];
*   int bytesRead = cellular->read(response, size, 10000);
*   response[size - 1] = '\0';
*   printf("READ: [%d] [%s]\r\n", bytesRead, response);
*
*   //Check to see if echo was successful
*   if (strcmp(data, response) == 0) {
*       printf("Echo Successful!\n\r");
*   } else {
*       printf("Echo failed!\n\r");
*   }
*
*   //Cleaning up the connection
*   printf("Closing socket: %s\r\n", cellular->close() ? "Success" : "Failure");
*   printf("Disconnecting...\r\n");
*   cellular->disconnect();
*   printf("End Program\n\r");
* }
* @endcode
*/

class Cellular : virtual mts::IPStack
{
public:

    /// An enumeration of radio registration states with a cell tower.
    enum Registration {
        NOT_REGISTERED, REGISTERED, SEARCHING, DENIED, UNKNOWN, ROAMING
    };

    /** This structure contains the data for an SMS message.
    */
    struct Sms {
        /// Message Phone Number
        std::string phoneNumber;
        /// Message Body
        std::string message;
        /// Message Timestamp
        std::string timestamp;
    };

    /** Destructs a Cellular object and frees all related resources.
    */
    ~Cellular();

    /** This static function is used to create or get a reference to a
    * Cellular object. Cellular uses the singleton pattern, which means
    * that you can only have one existing at a time. The first time you
    * call getInstance this method creates a new uninitialized Cellular
    * object and returns it. All future calls to this method will return
    * a reference to the instance created during the first call. Note that
    * you must call init on the returned instance before mnaking any other
    * calls. If using this class's bindings to any of the Socket package
    * classes like TCPSocketConnection, you must call this method and the
    * init method on the returned object first, before even creating the
    * other objects.
    *
    * @returns a reference to the single Cellular obect that has been created.
    */
    static Cellular* getInstance();

    /** This method initializes the object with the underlying radio
    * interface to use. Note that this function MUST be called before
    * any other calls will function correctly on a Cellular object. Also
    * note that MTSBufferedIO is abstract, so you must use one of
    * its inherited classes like MTSSerial or MTSSerialFlowControl.
    *
    * @param io the buffered io interface that is attached to the cellular
    * radio.
    * @param DCD this is the dcd signal from the radio. If attached the
    * the pin must be passed in here for this class to operate correctly.
    * The default is not connected.
    * @param DTR this is the dtr signal from the radio. If attached the
    * the pin must be passed in here for this class to operate correctly.
    * The default is not connected.
    * @returns true if the init was successful, otherwise false.
    */
    bool init(MTSBufferedIO* io, PinName DCD = NC, PinName DTR = NC);

    // Radio link related commands
    /** This method establishes a data connection on the cellular radio.
    * Note that before calling you must have an activated radio and if
    * using a SIM card set the APN using the setApn method. The APN can
    * be obtained from your cellular service provider.
    *
    * @returns true if the connection was successfully established, otherwise
    * false on an error.
    */
    virtual bool connect();

    /** This method is used to stop a previously established cellular data connection.
    */
    virtual void disconnect();

    /** This method is used to check if the radio currently has a data connection
    * established.
    *
    * @returns true if a data connection exists, otherwise false.
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

    //Other
    /** A method to reset the Multi-Tech Socket Modem.  This command brings down the
    * PPP link if it is up.  After this function is called, at least 30 seconds should
    * be allowed for the cellular radio to come back up before any other Cellular
    * functions are called.
    */
    /** this needs to be investigated.  After we tell the radio to reset and wait 30 seconds,
    * we can't seem to get it to respond to even a simple signal strength query.
    */
    virtual void reset();

    //Cellular Radio Specific
    /** A method for sending a generic AT command to the radio. Note that you cannot
    * send commands and have a data connection at the same time.
    *
    * @param command the command to send to the radio without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).  Does not append any character if esc == 0.
    * @returns all data received from the radio after the command as a string.
    */
    std::string sendCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** A method for sending a basic AT command to the radio. A basic AT command is
    * one that simply has a response of either OK or ERROR without any other information.
    * Note that you cannot send commands and have a data connection at the same time.
    *
    * @param command the command to send to the radio without the escape character.
    * @param timeoutMillis the time in millis to wait for a response before returning.
    * @param esc escape character to add at the end of the command, defaults to
    * carriage return (CR).
    * @returns the standard Code enumeration.
    */
    Code sendBasicCommand(const std::string& command, unsigned int timeoutMillis, char esc = CR);

    /** This method is used to get the IP address of the device, which is determined
    * via DHCP by the cellular carrier.
    *
    * @returns the devices IP address.
    */
    std::string getDeviceIP();
    
    /** A method for testing command access to the radio.  This method sends the
    * command "AT" to the radio, which is a standard radio test to see if you
    * have command access to the radio.
    *
    * @returns the standard AT Code enumeration.
    */
    Code test();

    /** A method for configuring command ehco capability on the radio. This command
    * sets whether sent characters are echoed back from the radio, in which case you
    * will receive back every command you send.
    *
    * @param state if true echo will be turned off, otherwise it will be turned on.
    * @returns the standard AT Code enumeration.
    */
    Code echo(bool state);

    /** A method for getting the signal strength of the radio. This method allows you to
    * get a value that maps to signal strength in dBm. Here 0-1 is Poor, 2-9 is Marginal,
    * 10-14 is Ok, 15-19 is Good, and 20+ is Excellent.  If you get a result of 99 the
    * signal strength is not known or not detectable.
    *
    * @returns an integer representing the signal strength.
    */
    int getSignalStrength();

    /** This method is used to check the registration state of the radio with the cell tower.
    * If not appropriatley registered with the tower you cannot make a cellular connection.
    *
    * @returns the registration state as an enumeration type.
    */
    Registration getRegistration();

    /** This method is used to set the radios APN if using a SIM card. Note that the APN
    * must be set correctly before you can make a data connection. The APN for your SIM
    * can be obtained by contacting your cellular service provider.
    *
    * @param the APN as a string.
    * @returns the standard AT Code enumeration.
    */
    Code setApn(const std::string& apn);

    /** This method is used to set the DNS which enables the use of URLs instead
    * of IP addresses when making a socket connection.
    *
    * @param the DNS server address as a string in form xxx.xxx.xxx.xxx.
    * @returns the standard AT Code enumeration.
    */
    Code setDns(const std::string& primary, const std::string& secondary = "0.0.0.0");

    /** This method is used test network connectivity by pinging a server.
    *
    * @param address the address of the server in format xxx.xxx.xxx.xxx.
    * @returns true if the ping was successful, otherwise false.
    */
    bool ping(const std::string& address = "8.8.8.8");

    /** This method can be used to trade socket functionality for performance.
    * In order to enable a socket connection to be closed by the client side programtically,
    * this class must process all read and write data on the socket to guard the special
    * escape character used to close an open socket connection. It is recommened that you
    * use the default of true unless the overhead of these operations is too significant.
    *
    * @param enabled set to true if you want the socket closeable, otherwise false. The default
    * is true.
    * @returns the standard AT Code enumeration.
    */
    Code setSocketCloseable(bool enabled = true);  //ETX closes socket (ETX and DLE in payload are escaped with DLE)

    /** This method is used to send an SMS message. Note that you cannot send an
    * SMS message and have a data connection open at the same time.
    *
    * @param phoneNumber the phone number to send the message to as a string.
    * @param message the text message to be sent.
    * @returns the standard AT Code enumeration.
    */
    Code sendSMS(const std::string& phoneNumber, const std::string& message);

    /** This method is used to send an SMS message. Note that you cannot send an
    * SMS message and have a data connection open at the same time.
    *
    * @param sms an Sms struct that contains all SMS transaction information.
    * @returns the standard AT Code enumeration.
    */
    Code sendSMS(const Sms& sms);

    /** This method retrieves all of the SMS messages currently available for
    * this phone number.
    *
    * @returns a vector of existing SMS messages each as an Sms struct.
    */
    std::vector<Cellular::Sms> getReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * even if they have never been retrieved or read.
    *
    * @returns the standard AT Code enumeration.
    */
    Code deleteAllReceivedSms();

    /** This method can be used to remove/delete all received SMS messages
    * that have been retrieved by the user through the getReceivedSms method.
    * Messages that have not been retrieved yet will be unaffected.
    *
    * @returns the standard AT Code enumeration.
    */
    Code deleteOnlyReceivedReadSms();

    /** A static method for getting a string representation for the Registration
    * enumeration.
    *
    * @param code a Registration enumeration.
    * @returns the enumeration name as a string.
    */
    static std::string getRegistrationNames(Registration registration);

private:
    static Cellular* instance; //Static pointer to the single Cellular object.

    MTSBufferedIO* io; //IO interface obect that the radio is accessed through.
    bool echoMode; //Specifies if the echo mode is currently enabled.

    bool pppConnected; //Specifies if a PPP session is currently connected.
    std::string apn; //A string that holds the APN for the radio.

    Mode mode; //The current socket Mode.
    bool socketOpened; //Specifies if a Socket is presently opened.
    bool socketCloseable; //Specifies is a Socket can be closed.
    unsigned int local_port; //Holds the local port for socket connections.
    std::string local_address; //Holds the local address for socket connections.
    unsigned int host_port; //Holds the remote port for socket connections.
    std::string host_address; //Holds the remote address for socket connections.
    DigitalIn* dcd; //Maps to the radios dcd signal
    DigitalOut* dtr; //Maps to the radios dtr signal

    Cellular(); //Private constructor, use the getInstance() method.
    Cellular(MTSBufferedIO* io); //Private constructor, use the getInstance() method.
};

}

#endif /* CELLULAR_H */
#include "mbed.h"
#include "M2XStreamClient.h"
#include "include_me.h"
#include "Websocket.h"
#include "MbedJSONValue.h"

using namespace mts;
Serial pc(USBTX, USBRX); // tx, rx
DigitalOut myled(LED1);
DigitalOut myled2(LED2);

const char key[] = "12988a511467b0686dad762d456cb5fc";  // enter your m2x user account master key
const char feed[] = "1e93ba89b35bc96e32fac1c43cc1d9f5"; // enter your blueprint feed id
const char stream[] = "amb-temp";   // Create a stream name

// set to 1 for cellular shield board
// set to 0 for wifi shield board
#define CELL_SHIELD 0

// ssid and phrase for wifi
std::string ssid = "belkin54g";
std::string phrase = "hackathon";
Wifi::SecurityType security_type = Wifi::WPA;
void on_data_point_found(const char* at, const char* value, int index, void* context) {
  pc.printf("Found a data point, index: %d\n", index);
  pc.printf("At: %s\n Value: %s\n", at, value);
}
int main()
{
    MTSSerial* serial = new MTSSerial(PTD3, PTD2, 256, 256);
    serial->baud(115200);
    Transport::setTransport(Transport::WIFI);
    Wifi* wifi = Wifi::getInstance();
    pc.printf("Init: %s\n\r", wifi->init(serial) ? "SUCCESS" : "FAILURE");
    pc.printf("Set Network: %s\n\r", getCodeNames(wifi->setNetwork(ssid, security_type, phrase)).c_str());
    pc.printf("Set DHCP: %s\n\r", getCodeNames(wifi->setDeviceIP("DHCP")).c_str());
    pc.printf("Signal Strnegth (dBm): %d\n\r", wifi->getSignalStrength());
    pc.printf("Is Connected: %s\n\r", wifi->isConnected() ? "True" : "False");
    pc.printf("Connect: %s\n\r", wifi->connect() ? "Success" : "Failure");
    pc.printf("Is Connected: %s\n\r", wifi->isConnected() ? "True" : "False");


    /* send some data */
    Client client;
    M2XStreamClient m2xClient(&client, key);
    int ret;
    while (true) {
    
        pc.printf("Receving\r\n");
        ret = m2xClient.receive(feed,stream,on_data_point_found,NULL);
        pc.printf("receive status %d\r\n", ret);
       // myled = 1;
       // myled2=0;
       // wait(0.2);
       // myled = 0;
       // myled2=1;
       // wait(0.2);
    }
    /*Websocket ws("ws://sockets.mbed.org:443/ws/pebblewand/rw");
    while (!ws.connect());
 
    while (1) {
            char *message=(char *)malloc(50);
            pc.printf("Receiving\n");
            if(ws.read(message))
                pc.printf(message);
       
            /*if(strcmp(message,"First Thing")==0)
                myled=1;
            else
                myled=0;
            wait(1.0);
    }*/
}
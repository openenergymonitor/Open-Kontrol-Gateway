/*

  Open Kontrol Gateway (OKG) Example  
  Receive data from an emonTx via RFM12B wireless then post online to emoncms using XN-XV Wifi Module
  
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3
  Authors: Trystan Lea and Glyn Hudson 
  Created: 14/08/12

  Use the following libraries:  
  https://github.com/harlequin-tech/WiFlyHQ
  https://github.com/openenergymonitor/jeelib
  
  For full step-by-step instructions See: http://wiki.openenergymonitor.org/index.php?title=Open_Kontrol_Gateway 
 
  Credits:
  Based on wifly http client by Harlequin 
  Uses modified Jeelabs.org JeeLib library for RFM12B 

 
*/

#include <WiFlyHQ.h>
#include <SoftwareSerial.h>        
SoftwareSerial wifiSerial(5,6);       //Using RH Xbee socket on OKG with two RH jumpers connected vertically. e.g: http://wiki.openenergymonitor.org/images/thumb/OKG1_PCB_Assembled.jpg/400px-OKG1_PCB_Assembled.jpg

#include <JeeLib.h>

//#include <AltSoftSerial.h>        //included as standard in Arduino 1.0
//AltSoftSerial wifiSerial(5,6);

//------------------------------------------------------------------------------------------------------
// RFM12B Wireless Config
//------------------------------------------------------------------------------------------------------
#define MYNODE 30            // node ID of OKG 
#define freq RF12_433MHZ     // frequency - must match RFM12B module and emonTx
#define group 210            // network group - must match emonTx 
//------------------------------------------------------------------------------------------------------

typedef struct { int power1, power2, power3, voltage; } PayloadTX;
PayloadTX emontx;

typedef struct { int temperature; } PayloadGLCD;
PayloadGLCD emonglcd;

//------------------------------------------------------------------------------------------------------
// RN-XV wifi Config & emoncms
//------------------------------------------------------------------------------------------------------
WiFly wifly;
void terminal();

/* Change these to match your WiFi network */
const char mySSID[] = "YOURSSID";
const char myPassword[] = "YOUR WIFI PASSWORD";	//leave blank for non

// Enter your apiurl here including apikey:
char apiurl[] = "http://emoncms.org/api/post.json?apikey=YOURAPIKEY&json=";

// For posting to emoncms server with host name
// emoncms.org is the public emoncms server. Emoncms can also be downloaded and run on any server.
const char server[] = "emoncms.org";                //emoncms server 
//------------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------------
// Open Kontrol Gateway Config
//------------------------------------------------------------------------------------------------------
const int LEDpin=17;         //front status LED on OKG
//------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------
// The PacketBuffer class is used to generate the json string that is send via ethernet - JeeLabs
//------------------------------------------------------------------------------------------------------
class PacketBuffer : public Print {
public:
    PacketBuffer () : fill (0) {}
    const char* buffer() { return buf; }
    byte length() { return fill; }
    void reset()
    { 
      memset(buf,NULL,sizeof(buf));
      fill = 0; 
    }
    virtual size_t write (uint8_t ch)
        { if (fill < sizeof buf) buf[fill++] = ch; }
    byte fill;
    char buf[150];
    private:
};
PacketBuffer str;
//--------------------------------------------------------------------------------------------------------
int emonglcd_rx = 0;                      // Used to indicate that emonglcd data is available
int data_ready, rf_error;
unsigned long last_rf;

//------------------------------------------------------------------------------------------------------
// SETUP
//------------------------------------------------------------------------------------------------------
void setup()
{
    char buf[32];

    Serial.begin(9600);
    Serial.println("openenergymonitor.org RFM12B > OKG > RN-XV wifi > emoncms");
    Serial.print("Free memory: ");
    Serial.println(wifly.getFreeMemory(),DEC);

  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin,HIGH);
  
  rf12_set_cs(9);                                                 //RFM12B CS connnected to Dig 9 on OKG - change to 10 for JeeNode/emonTx ...
  rf12_initialize(MYNODE, freq,group);
  last_rf = millis()-40000;                                       // setting lastRF back 40s is useful as it forces the ethernet code to run straight away
  
   // print RFM12B settings 
  Serial.print("Node: "); 
  Serial.print(MYNODE); 
  Serial.print(" Freq: "); 
   if (freq == RF12_433MHZ) Serial.print("433Mhz");
   if (freq == RF12_868MHZ) Serial.print("868Mhz");
   if (freq == RF12_915MHZ) Serial.print("915Mhz"); 
  Serial.print(" Network: "); 
  Serial.println(group);
  
    wifiSerial.begin(9600);
    if (!wifly.begin(&wifiSerial, &Serial)) {
        Serial.println("Failed to start wifly");
	terminal();
    }

    /* Join wifi network if not already associated */
    if (!wifly.isAssociated()) {
	/* Setup the WiFly to connect to a wifi network */
	Serial.println("Joining network");
	wifly.setSSID(mySSID);
	wifly.setPassphrase(myPassword);
	wifly.enableDHCP();

	if (wifly.join()) {
	    Serial.println("Joined wifi network");
	} else {
	    Serial.println("Failed to join wifi network");
	    terminal();
	}
    } else {
        Serial.println("Already joined network");
    }
    //terminal();

    Serial.print("MAC: ");
    Serial.println(wifly.getMAC(buf, sizeof(buf)));
    Serial.print("IP: ");
    Serial.println(wifly.getIP(buf, sizeof(buf)));
    Serial.print("Netmask: ");
    Serial.println(wifly.getNetmask(buf, sizeof(buf)));
    Serial.print("Gateway: ");
    Serial.println(wifly.getGateway(buf, sizeof(buf)));

    wifly.setDeviceID("Wifly-WebClient");
    Serial.print("DeviceID: ");
    Serial.println(wifly.getDeviceID(buf, sizeof(buf)));

    

}

void loop()
{
   //-----------------------------------------------------------------------------------------------------------------
  // 1) Receive date from emonTx via RFM12B
  //-----------------------------------------------------------------------------------------------------------------
    if (rf12_recvDone()){      
      if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)
      {
        last_rf = millis();                                              // reset lastRF timer
        int node_id = (rf12_hdr & 0x1F);				 // extract nodeID from payload
        
        if (node_id == 10)                                    // Check if date is coming from emonTx, set nodeID at start sketch
        {
          digitalWrite(LEDpin,HIGH);				         //turn on status LED
          emontx = *(PayloadTX*) rf12_data;                              // get emontx data                                                            
 
          str.reset();                                                   // Reset json string      
          str.print("{rf_fail:0");                                       // RF recieved so no failure
          str.print(",power1:");       str.print(emontx.power1);         // Add power reading
          str.print(",power2:");       str.print(emontx.power2);         // Add power reading
          str.print(",power3:");       str.print(emontx.power3);         // Add power reading
          str.print(",voltage:");      str.print(emontx.voltage);        // Add emontx battery voltage reading      

          data_ready = 1;                                                // data is ready
          rf_error = 0;
        }
        
        if (node_id == 20)                                               // EMONGLCD 
        {
          emonglcd = *(PayloadGLCD*) rf12_data;                          // get emonglcd data
          emonglcd_rx = 1;        
        } 
        
      }
    }
     
  //-----------------------------------------------------------------------------------------------------------------
  // 2) If no data is recieved from rf12 emoncms is updated every 30s with RFfail = 1 indicator for debugging
  //-----------------------------------------------------------------------------------------------------------------
  if ((millis()-last_rf)>30000)
  {
    last_rf = millis();                                                 // reset lastRF timer
    str.reset();                                                        // reset json string
    str.print("{rf_fail:1");                                            // No RF received in 30 seconds so send failure 
    data_ready = 1;                                                     // Ok, data is ready
    rf_error=1;
  }

  //-----------------------------------------------------------------------------------------------------------------
 // 3) Post Data
 //-----------------------------------------------------------------------------------------------------------------
  if (data_ready) {
    
    // include temperature data from emonglcd if it has been recieved  
    if (emonglcd_rx) {
      str.print(",temperature:");  
      str.print(emonglcd.temperature/100.0);
      emonglcd_rx = 0;
    }
      
    //if (client.connect(server, 80)) {
      //str.print("}\0");
      //Serial.print("Sent: "); Serial.println(str.buf);
      //client.print("GET "); client.print(apiurl); client.print(str.buf); client.println();
      
      if (wifly.isConnected()) {
        //Serial.println("Old connection active. Closing");
	wifly.close();
    }

    if (wifly.open(server, 80)) {
        str.print("}\0");
        Serial.print("Connected to ");
	Serial.println(server);
        Serial.print("Sent: "); Serial.println(str.buf);
	/* Send the request */
	wifly.print("GET "); wifly.print(apiurl); wifly.print(str.buf);
	wifly.println();
    } else 
        Serial.println("Failed to connect");
     
      delay(300);
      data_ready=0;
      digitalWrite(LEDpin,LOW);		  // turn off status LED to indicate succesful data receive and online posting
    } 

 //-----------------------------------------------------------------------------------------------------------------
 // 4) Read Reply 
 //-----------------------------------------------------------------------------------------------------------------
    if (wifly.available() > 0) {
	char ch = wifly.read();
	Serial.write(ch);
	if (ch == '\n') {
	    /* add a carriage return */ 
	    Serial.write('\r');
	}
    }

    if (Serial.available() > 0) {
	wifly.write(Serial.read());
    } 
}

/* Connect the WiFly serial to the serial monitor. */
void terminal()
{
    while (1) {
	if (wifly.available() > 0) {
	    Serial.write(wifly.read());
	}


	if (Serial.available() > 0) {
	    wifly.write(Serial.read());
	}
    }
}

/*
Open Kontrol Gateway (OKG) Example  
Receive data from an emonTx via RFM12B wireless then post online to emoncms using Wiznet 5200 Ethernet Module
  
Part of the openenergymonitor.org project
Licence: GNU GPL V3
Authors: Trystan Lea and Glyn Hudson 
Created: 09/04/2012, updated 08/07/12 

Use the following librarys: 
https://github.com/openenergymonitor/Ethernet
https://github.com/openenergymonitor/jeelib
  
For full step-by-step instructions See: http://wiki.openenergymonitor.org/index.php?title=Open_Kontrol_Gateway 
 
 Credits:
 Based on Arduino DNS and DHCP-based Web client by David A. Mellis
 modified 12 April 2011by Tom Igoe, based on work by Adrian McEwen
 Uses Jeelabs.org JeeLib library for RFM12B 

Example will also work with Arduino Ethernet, Arduino + newer Ethernet shields with addition of RFM12B. Bug in older Ethernet shields stops RFM12B and Wiznet being using together

 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <JeeLib.h>	     //https://github.com/jcw/jeelib


//------------------------------------------------------------------------------------------------------
// RFM12B Wireless Config
//------------------------------------------------------------------------------------------------------
#define MYNODE 30            //node ID of OKG 
#define freq RF12_433MHZ     //frequency - must match RFM12B module and emonTx
#define group 210            //network group - must match emonTx 

const int emonTx_NodeID=10;  //set to match emonTx node ID

typedef struct { int power, apparentPower, Vrms, heaterPower, heaterApparent, gen, apparentGen, Livingroom, Outside, pulse; } PayloadTX;    //Structure must match that being sent from emonTx
PayloadTX emontx;

//------------------------------------------------------------------------------------------------------
 

//------------------------------------------------------------------------------------------------------
// Ethernet Config
//------------------------------------------------------------------------------------------------------
byte mac[] = {  0x00, 0xAB, 0xBB, 0xCC, 0xDE, 0x02 }; //OKG MAC - experiment with different MAC addresses if you have trouble connecting 
byte ip[] = {192, 168, 1, 99 };                       //OKG static IP - only used if DHCP failes

char server[] = "emoncms.org";         // for posting to emoncms server with host name, (DNS lookup) comment out if using static IP address below
// vis.openenergymonitor.org is the public emoncms demo server. emoncms can also be downloaded and ran on any server

//IPAddress server(xxx,xxx,xxx,xxx);                 // - emoncms server IP for posting to server without a host name, can be used for posting to local emoncms server

EthernetClient client;
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

int data_ready, rf_error;
unsigned long last_rf;


//------------------------------------------------------------------------------------------------------
// SETUP
//------------------------------------------------------------------------------------------------------
void setup() {

  Serial.begin(9600);
  Serial.println("openenergymonitor.org RFM12B > OKG > Wiznet, > emoncms");
  
  pinMode(LEDpin, OUTPUT);
  digitalWrite(LEDpin,HIGH);
  
  rf12_set_cs(9);
  rf12_initialize(MYNODE, freq,group);
  last_rf = millis()-40000;                                       // setting lastRF back 40s is useful as it forces the ethernet code to run straight away
  
  
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    Ethernet.begin(mac, ip);                                      //configure manually 
  }
  
  // print your local IP address:
  Serial.print("Local IP address: ");
  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print("."); 
  }
  Serial.println();
  
 // print RFM12B settings 
  Serial.print("Node: "); 
  Serial.print(MYNODE); 
  Serial.print(" Freq: "); 
   if (freq == RF12_433MHZ) Serial.print("433Mhz");
   if (freq == RF12_868MHZ) Serial.print("868Mhz");
   if (freq == RF12_915MHZ) Serial.print("915Mhz"); 
  Serial.print(" Network: "); 
  Serial.println(group);
  
  delay(200);
  digitalWrite(LEDpin,LOW);	//turn of OKG status LED to indicate setup success 
  
}
//------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------
// LOOP
//------------------------------------------------------------------------------------------------------
void loop()
{

  //-----------------------------------------------------------------------------------------------------------------
  // 1) Receive date from emonTx via RFM12B
  //-----------------------------------------------------------------------------------------------------------------
    if (rf12_recvDone()){      
      if (rf12_crc == 0 && (rf12_hdr & RF12_HDR_CTL) == 0)
      {
        int node_id = (rf12_hdr & 0x1F);				//extract nodeID from payload
        
        if (node_id == emonTx_NodeID)                                    // Check if date is coming from emonTx, set nodeID at start sketch
        {
          digitalWrite(LEDpin,HIGH);				         //turn on status LED
          emontx = *(PayloadTX*) rf12_data;                              // get emontx data                                          
          Serial.print("emontx datra rx ");				 // print emontx data to serial
          last_rf = millis();                                            // reset lastRF timer
          
          delay(50);                                                     // make sure serial printing finished
                               
          // JSON creation: JSON sent are of the format: {key1:value1,key2:value2} and so on
          
          str.reset();                                                   // Reset json string      
          str.print("{rf_fail:0");                                       // RF recieved so no failure
          str.print(",BF_Rp:");        str.print(emontx.power);          // Add power reading
          str.print(",BF_Ap:");        str.print(emontx.apparentPower);          // Add power reading
          str.print(",vrms1:");        str.print(emontx.Vrms);          // Add power reading 
          str.print(",BF_RpH:");      str.print(emontx.heaterPower);        // Add emontx battery voltage reading
          str.print(",BF_ApH:");      str.print(emontx.heaterApparent);
          str.print(",BF_RpS:");      str.print(emontx.gen);
          str.print(",BR_ApS:");      str.print(emontx.apparentGen);
          str.print(",BF_Tl:");      str.print((emontx.Livingroom/100.0));
          str.print(",BF_To:");      str.print((emontx.Outside/100.0));
          
          
          Serial.println(str.buf);
    
          data_ready = 1;                                                // data is ready
          rf_error = 0;
          
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
  if (client.connect(server, 80)) {
    Serial.println("connected");
    str.print("}\0");  			//  End of json string
    client.print("GET /emoncms3/api/post.json?apikey=xxxxxxxxxxxxxxxxxxxxxxxxxxxxx&json=");	//enter your emoncms write only API key here
    client.print(str.buf);		//send the data
    client.println();
    delay(300);
    client.stop();			//disconnect 
    data_ready=0;
    digitalWrite(LEDpin,LOW);		//turn off status LED to indicate succesful data receive and online posting
  } 
  else { 
    Serial.println("connection failed"); // if no connection you didn't get a connection to the server:
    delay(1000);			//wait 2s before trying again 
  }
}

}//end loop

//------------------------------------------------------------------------------------------------------


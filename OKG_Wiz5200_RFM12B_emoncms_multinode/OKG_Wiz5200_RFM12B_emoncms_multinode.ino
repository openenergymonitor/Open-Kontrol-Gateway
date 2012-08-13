/*

  Open Kontrol Gateway (OKG) Example  
  Receive data from an emonTx via RFM12B wireless then post online to emoncms using Wiznet 5200 Ethernet Module
  
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3
  Authors: Trystan Lea and Glyn Hudson 
  Created: 13/08/12

  Use the following libraries:  
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
#include <JeeLib.h>

//------------------------------------------------------------------------------------------------------
// RFM12B Wireless Config
//------------------------------------------------------------------------------------------------------
#define MYNODE 30            // node ID of OKG 
#define freq RF12_433MHZ     // frequency - must match RFM12B module and emonTx
#define group 210            // network group - must match emonTx 
//------------------------------------------------------------------------------------------------------
 
//------------------------------------------------------------------------------------------------------
// Ethernet Config
//------------------------------------------------------------------------------------------------------
byte mac[] = { 0x00, 0xAB, 0xBB, 0xCC, 0xDE, 0x02 };  // OKG MAC - experiment with different MAC addresses if you have trouble connecting 
byte ip[] = {192, 168, 1, 99 };                       // OKG static IP - only used if DHCP failes

// Enter your apiurl here including apikey:
char apiurl[] = "http://emoncms.org/api/post.json?apikey=YOURAPIKEY";

// For posting to emoncms server with host name, (DNS lookup) comment out if using static IP address below
// emoncms.org is the public emoncms server. Emoncms can also be downloaded and run on any server.
char server[] = "emoncms.org";    

//IPAddress server(xxx,xxx,xxx,xxx);                  // emoncms server IP for posting to server without a host name, can be used for posting to local emoncms server

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
  Serial.println("openenergymonitor.org RFM12B > OKG > Wiznet, > emoncms MULTI-NODE");
  
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
        int node_id = (rf12_hdr & 0x1F);
        byte n = rf12_len;
         
        str.reset();
        str.print("&node=");  str.print(node_id);
        str.print("&csv=");
        for (byte i=0; i<n; i+=2)
        {
          int num = ((unsigned char)rf12_data[i+1] << 8 | (unsigned char)rf12_data[i]);
          if (i) str.print(",");
          str.print(num);
        }

        str.print("\0");  //  End of json string
        data_ready = 1; 
        last_rf = millis(); 
        rf_error=0;
      }
  }
     
  //-----------------------------------------------------------------------------------------------------------------
  // 2) If no data is recieved from rf12 emoncms is updated every 30s with RFfail = 1 indicator for debugging
  //-----------------------------------------------------------------------------------------------------------------
  if ((millis()-last_rf)>30000)
  {
    last_rf = millis();                                                 // reset lastRF timer
    str.reset();                                                        // reset json string
    str.print("&json={rf_fail:1}\0");                                            // No RF received in 30 seconds so send failure 
    data_ready = 1;                                                     // Ok, data is ready
    rf_error=1;
  }

 //-----------------------------------------------------------------------------------------------------------------
 // 3) Post Data
 //-----------------------------------------------------------------------------------------------------------------
if (data_ready) {
  if (client.connect(server, 80)) {
    
    Serial.print("Sent: "); Serial.println(str.buf);
    client.print("GET "); client.print(apiurl); client.print(str.buf); client.println();
    
    delay(300);
    client.stop();			  // disconnect 
    data_ready=0;
    digitalWrite(LEDpin,LOW);		  // turn off status LED to indicate succesful data receive and online posting
  } 
  else { 
    Serial.println("connection failed");  // if no connection you didn't get a connection to the server:
    delay(1000);			  // wait 1s before trying again 
  }
}

}//end loop

//------------------------------------------------------------------------------------------------------


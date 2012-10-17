/*

  Open Kontrol Gateway (OKG) Example  
  Receive data from an emonTx via RFM12B wireless then post online to emoncms using Wiznet 5200 Ethernet Module
  
  Part of the openenergymonitor.org project
  Licence: GNU GPL V3
  Authors: Trystan Lea and Glyn Hudson 
  Created: 09/04/2012, updated 08/07/12

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
#include <JeeLib.h>	     //https://github.com/jcw/jeelib

//------------------------------------------------------------------------------------------------------
// RFM12B Wireless Config
//------------------------------------------------------------------------------------------------------
#define MYNODE 15            // node ID of OKG 
#define freq RF12_433MHZ     // frequency - must match RFM12B module and emonTx
#define group 210            // network group - must match emonTx 
//------------------------------------------------------------------------------------------------------
 
typedef struct { int power1, power2, power3, voltage; } PayloadTX;
PayloadTX emontx;

typedef struct { int temperature; } PayloadGLCD;
PayloadGLCD emonglcd;
 
//------------------------------------------------------------------------------------------------------
// Ethernet Config
//------------------------------------------------------------------------------------------------------
byte mac[] = { 0x00, 0xAB, 0xBB, 0xCC, 0xDE, 0x02 };  // OKG MAC - experiment with different MAC addresses if you have trouble connecting 
byte ip[] = {192, 168, 1, 99 };                       // OKG static IP - only used if DHCP failes

// Enter your apiurl here including apikey:
char apiurl[] = "http://emoncms.org/api/post.json?apikey=YOURAPIKEY&json=";
char timeurl[] = "http://emoncms.org/time/local.json?apikey=YOURAPIKEY";
// For posting to emoncms server with host name, (DNS lookup) comment out if using static IP address below
// emoncms.org is the public emoncms server. Emoncms can also be downloaded and run on any server.
char server[] = "emoncms.org";    

//IPAddress server(xxx,xxx,xxx,xxx);                  // emoncms server IP for posting to server without a host name, can be used for posting to local emoncms server

EthernetClient client;
const int WizResetPin = 7;                             // wired to the Wiznet reset line
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
unsigned long last_rf, time60s;
boolean lastConnected = false;

char line_buf[50];                        // Used to store line of http reply header
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
  last_rf = millis()-40000;     // setting lastRF back 40s is useful as it forces the ethernet code to run straight away
  
  pinMode(WizResetPin, OUTPUT);        // Reset the Wiznet module
  digitalWrite(WizResetPin, LOW);                
  delay(5);
  digitalWrite(WizResetPin, HIGH);
  
  
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
  
  wdt_enable(WDTO_8S);                //enable an 8's reset watchdog
  
}
//------------------------------------------------------------------------------------------------------


//------------------------------------------------------------------------------------------------------
// LOOP
//------------------------------------------------------------------------------------------------------
void loop()
{
  if (client.available())
  {
    memset(line_buf,NULL,sizeof(line_buf));
    int pos = 0;
    
    while (client.available()) {
      char c = client.read();
      line_buf[pos] = c;
      pos++;
    }  

    if (strcmp(line_buf,"ok")==0)
    {
      Serial.println("OK recieved");
    }
    else if(line_buf[0]=='t')
    { 
      Serial.print("Time: ");
      Serial.println(line_buf);
    
      char tmp[] = {line_buf[1],line_buf[2],0};
      byte hour = atoi(tmp);
      tmp[0] = line_buf[4]; tmp[1] = line_buf[5];
      byte minute = atoi(tmp);
      tmp[0] = line_buf[7]; tmp[1] = line_buf[8];
      byte second = atoi(tmp);

      if (hour>0 || minute>0 || second>0) 
      {  
        char data[] = {'t',hour,minute,second};
        int i = 0; while (!rf12_canSend() && i<10) {rf12_recvDone(); i++;}
        rf12_sendStart(0, data, sizeof data);
        rf12_sendWait(0);
      }
    }
  }
  
  if (!client.connected() && lastConnected) {
    Serial.println();
    Serial.println("disconnecting.");
    client.stop();
  }
  
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
  if (!client.connected() && data_ready) {
    
    if (emonglcd_rx) {
      str.print(",temperature:");  
      str.print(emonglcd.temperature/100.0);
      emonglcd_rx = 0;
    }

    if (client.connect(server, 80)) {
      str.print("}\0");
      Serial.println();
      Serial.print("Sent: "); Serial.println(str.buf);
      client.print("GET "); client.print(apiurl); client.print(str.buf); client.println();
    
      delay(300);
    
      data_ready=0;
      digitalWrite(LEDpin,LOW);		  // turn off status LED to indicate succesful data receive and online posting
    } 
    else {Serial.println("Cant connect to send data"); delay(500); client.stop();}
  }

  if (!client.connected() && ((millis()-time60s)>10000))
  {
    time60s = millis();                                                 // reset lastRF timer

    if (client.connect(server, 80)) {
      Serial.println();
      Serial.println("Sent time request");
      client.print("GET "); client.print(timeurl); client.println();
    }
    else {Serial.println("Cant connect to req time"); delay(500); client.stop();}

  }

  lastConnected = client.connected();

wdt_reset();  
}//end loop

//------------------------------------------------------------------------------------------------------


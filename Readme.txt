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

Note: this code is a work in progress...

It would be good to add:
Watchdog timer to reset the Atmega328 in case of crash 
Callback function to establish that data is actually being received by emoncms
emonGLCD compatablity - get current time from server and transmit to emonGLCD
Local SD card logging option or SRAM logging to log data in event of no Etherent connectivity 
Suppport for Roving Network RN-XV wifi module 
Web config 

Open Kontrol Gateway (OKG) Example  
Receive data from an emonTx via RFM12B wireless then post online to emoncms using Wiznet 5200 Ethernet Module
  
Part of the openenergymonitor.org project
Licence: GNU GPL V3
Authors: Trystan Lea and Glyn Hudson 
Created: 09/04/2012 

  OKG - http://shop.ciseco.co.uk/openkontrol-gateway/
  Wiznet W5200 module - http://shop.ciseco.co.uk/openkontrol-gateway-wiznet-ethernet-kit/
  OKG Documentation - http://openmicros.org/index.php/articles/92-ciseco-product-documentation/openkontrol-gateway
  emonTx - shop.openenergymonitor.com/transmitters/
  emonTx documentation - http://openenergymonitor.org/emon/emontx 
  emoncms http://openenergymonitor.org/emon/emoncms 
  
  Sketch will also work with Arduino Ethernet, Arduino + newer Ethernet shields with addition of RFM12B. 
  Bug in older Ethernet shields stops RFM12B and Wiznet being using together
  
 Modifications are required to JeeLib RFM12.cpp, Arduino Ethernet library needs to be updated to use Wiznet 5200
 Ethernet library needs to be modified to disable interrupts to enable RFM12B and Ethernet to work at the same time. 
 Thanks to John Crouchley for these modifications : - 

	1.) For the Ciseco OKG Gateway we have to change RFM12B CS pin. This is in the file RF12.cpp in the JeeLib library. Under 'ATmega168, ATmega328, etc.' section

	change
	#define SS_BIT      2     // For JeeNodes and emonTx - for PORTB: 2 = d.10, 1 = d.9, 0 = d.8

	to
	#define SS_BIT      1     // For OKG


	2.) Download Wiznet W5200 Arduino Library update from: http://wiznettechnology.com/sub_modules/en/product/product_detail.asp?Refid=491&page=1&cate1=5&cate2=42&cate3=0&pid=1161&cType=2#tab
	Override files in Libraries/Ethernet/utility

	3.) Ethernet library needs to be modified to disable interrupts to enable RFM12B and Wiznet to work at the same time
	Code  is in Ethernet\utility\w5100.h  :-
	#if defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)

	  inline static void initSS()    { DDRB  |=  _BV(4); };

	  inline static void setSS()     { cli(); PORTB &= ~_BV(4); };

	  inline static void resetSS()   { PORTB |=  _BV(4); sei(); };

	#elif defined(__AVR_ATmega32U4__) || defined(__AVR_AT90USB1286__) || defined(__AVR_AT90USB646__) || defined(__AVR_AT90USB162__)

	  inline static void initSS()    { DDRB  |=  _BV(0); };

	  inline static void setSS()     { cli(); PORTB &= ~_BV(0); };

	  inline static void resetSS()   { PORTB |=  _BV(0); sei(); };

	#else

	  inline static void initSS()    { DDRB  |=  _BV(2); };

	  inline static void setSS()     { cli(); PORTB &= ~_BV(2); };

	  inline static void resetSS()   { PORTB |=  _BV(2); sei(); };

	#endif

	The addition of the cli(); and the sei(); are the only changes.

 
 Credits:
 Based on Arduino DNS and DHCP-based Web client by David A. Mellis
 modified 12 April 2011by Tom Igoe, based on work by Adrian McEwen
 Uses Jeelabs.org JeeLib library for RFM12B 


Note: this code is a work in progress...

It would be good to add:
Watchdog timer to reset the Atmega328 in case of crash 
Callback function to establish that data is actually being received by emoncms
emonGLCD compatablity - get current time from server and transmit to emonGLCD
Local SD card logging option or SRAM logging to log data in event of no Etherent connectivity 
Suppport for Roving Network RN-XV wifi module 
Web config 

#include <RF24.h>
#include <SPI.h>
#include <Ethernet.h>
#include <TextFinder.h>
#include <EEPROM.h>
#include <Streaming.h>
#include <avr/pgmspace.h>
#include <ATEMstd.h>
//#include <ATEMTally.h>

#include "ATEMData.h"

/********** RADIO SETUP ************/
const bool radioNumber = 0;

const int RADIO_CHIP_ENABLED_PIN = 7;
const int RADIO_CHIP_SELECT_PIN = 8;

RF24 radio(RADIO_CHIP_ENABLED_PIN, RADIO_CHIP_SELECT_PIN);

const uint8_t address[] = { 0x10,0x10,0x10 };
/*********** /RADIO SETUP **********/

/********** IP SETUP ************/
// Set the MAC address of the arduino.
const byte mac[] = { 0x90, 0xA2, 0xDA, 0x0E, 0xD3, 0xAB };

// Set the IP address of the arduino client.
const IPAddress clientIp(192, 168, 178, 239);

// set the default IP address of the ATEM switcher
const IPAddress switcherIp(192, 168, 178, 240);

// set the default PORT of the ATEM switcher
const int switcher_port = 49910;

// initialize the ethernet server (for settings page)
//EthernetServer server(80);

// set to false initially; set to true when ATEM switcher initializes
boolean ranOnce = false;

// create a new AtemSwitcher and ATEMTally object
ATEMstd AtemSwitcher;
//ATEMTally ATEMTally;

// Define variables for determining whether tally is on
boolean programOn;
boolean previewOn;

Payload payload;
Payload oldPayload;

// Timestamp of last radio tx.
unsigned long lastRadioTx = 0;

void setup()
{
  // Start the Ethernet, Serial (debugging) and UDP:
  Ethernet.begin(mac,clientIp);
  Serial.begin(115200);
  Serial << F("\n- - - - - - - -\nSerial Started\n");  
  
	// initialize the RF12 radio; set the Node # to 20
	radio.begin();
  // 3 bytes are enough.
  radio.setAddressWidth(3);
  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_MAX);
  // We do multicast. Disable ACK this could lead to problems (usually on the receiving side)
  // but just to be shure.
  radio.setAutoAck(false);
  // Slowest rate, totally okay for us. Most secure against interference.
  radio.setDataRate(RF24_250KBPS);
  radio.openWritingPipe(address);

  // Initialize a connection to the switcher:
  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();

	// initialize the ATEMTally object
	//ATEMTally.initialize();
	
	// set the LED to RED
	//ATEMTally.change_LED_state(2);
	
	// setup the Ethernet
	//ATEMTally.setup_ethernet(mac, ip, switcher_ip, switcher_port);
	
	// start the server
	//server.begin();
	// Init the structs.
	memset(&payload, 0, sizeof payload);
	memset(&oldPayload, 0, sizeof oldPayload);

  // set this variable to current time
  lastRadioTx = millis();

	// delay a bit before looping
	delay(1000);
}

void loop()
{
	// AtemSwitcher function for retrieving the program and preview camera numbers
	AtemSwitcher.runLoop();

	// if connection is gone anyway, try to reconnect
	if (AtemSwitcher.hasInitialized())  {
		// Reset the previous program and preview numbers
		payload.program_1 = 0;
		payload.program_2 = 0;
		payload.preview = 0;

		// assign the program and preview numbers to the payload structure
		for (int i = 1; i <= 16; i++) {
			programOn = AtemSwitcher.getProgramTally(i);
			previewOn = AtemSwitcher.getPreviewTally(i);

      // There are two program channels which can be on. So we must
      // check both.
			if (programOn) {
				if (payload.program_1 == 0) {
					payload.program_1 = i;
				} else {
					payload.program_2 = i;
				}
			}
     
			if (previewOn) {
				payload.preview = i;
			}
		}

		// when radio is available and new data here transmit or at least transmit
    // every 5 seconds.
		if (!isPayloadEqual(payload, oldPayload) || (millis() - lastRadioTx > 5000)) {
		  
		  Serial << F("Trans Prev: ") << payload.preview << " Prog: " << payload.program_1 << "\n";  
      
      radio.stopListening();
			radio.write(&payload, sizeof(payload));
			
			oldPayload = payload;
			//ATEMTally.change_LED_state(3);

      lastRadioTx = millis();
		}
	}

	// a delay is needed due to some weird issue
	delay(10);

	//ATEMTally.change_LED_state(1);

	// monitors for the reset button press
	//ATEMTally.monitor_reset();
}


#include <RF24.h>
#include "ATEMData.h"

// PROGRAM LED pin
const int PROGRAM_PIN = A0;
// PREVIEW LED pin
const int PREVIEW_PIN = A2;
// POWER LED pin
const int POWER_PIN = A1;

// Node # DIP switch 1 pin
const int DIP1_PIN = 3;
// Node # DIP switch 2 pin
const int DIP2_PIN = 4;
// Node # DIP switch 3 pin
const int DIP3_PIN = 5;
// Node # DIP switch 4 pin
const int DIP4_PIN = 6;

// last received radio signal time (to power off LEDs when no signal)
unsigned long last_radio_recv = 0;

// default Node # 200 (alias to 0) will blink constantly if signal exists
int this_node = 200;

// Holding the received data.
Payload payload;

// Holding currently displayed program.
Payload currentPayload;



/********** RADIO SETUP ************/
const bool radioNumber = 0;

const int RADIO_CHIP_ENABLED_PIN = 9;
const int RADIO_CHIP_SELECT_PIN = 10;

RF24 radio(RADIO_CHIP_ENABLED_PIN, RADIO_CHIP_SELECT_PIN);

const uint8_t address[] = { 0x10,0x10,0x10 };
/***********************************/

void setup() {
	// initialize all the defined pins
	pinMode(PROGRAM_PIN, OUTPUT);
	pinMode(PREVIEW_PIN, OUTPUT);
	pinMode(POWER_PIN, OUTPUT);
	pinMode(DIP1_PIN, INPUT_PULLUP);
  pinMode(DIP2_PIN, INPUT_PULLUP);
  pinMode(DIP3_PIN, INPUT_PULLUP);
  pinMode(DIP4_PIN, INPUT_PULLUP);

	// turn all LEDs off
  digitalWrite(PROGRAM_PIN, LOW);
  digitalWrite(PREVIEW_PIN, LOW);
  digitalWrite(POWER_PIN, LOW); 
  
	// set the Node # according to the DIP pins
  setNodeID();

  // initialize the radio
  radio.begin();

  // 3 bytes are enough.
  radio.setAddressWidth(3);

  // Set the PA Level low to prevent power supply related issues since this is a
  // getting_started sketch, and the likelihood of close proximity of the devices. RF24_PA_MAX is default.
  radio.setPALevel(RF24_PA_LOW);

  // We do multicast. Disable ACK this could lead to problems (usually on the receiving side)
  // but just to be shure.
  radio.setAutoAck(false);

  // Slowest rate, totally okay for us. Most secure against interference.
  radio.setDataRate(RF24_250KBPS);

  radio.openReadingPipe(0, address);

  radio.startListening();
  
	if (this_node == 200) {
		// if the Node # is 0 (alias to 200), blink quickly (30 times) on power on
		for (int i=0; i < 30; i++) {
			digitalWrite(POWER_PIN, HIGH);
			delay(20);
			digitalWrite(POWER_PIN, LOW);
			delay(20);
		}
	} else {
		// if the Node # is a set number, blink the Node # on power on
		for (int i=0; i < this_node; i++) {
			digitalWrite(POWER_PIN, HIGH);
			delay(300);
			digitalWrite(POWER_PIN, LOW);
			delay(300);
		}
	}
  
	// set this variable to current time
  last_radio_recv = millis();
}

void loop() {

  if(radio.available()) {
    radio.read(&payload, sizeof(payload) );

    // Receiving of a new packed was done.
    // Check if it contains new data.
    if(isPayloadEqual(payload, currentPayload)) {
      return;
    }
    
    // get the program and preview numbers
    currentPayload = payload;

    // keep track of last radio signal time
    last_radio_recv = millis();

    // turn off all LEDs
    digitalWrite(PREVIEW_PIN, LOW);
    digitalWrite(PROGRAM_PIN, LOW);
    digitalWrite(POWER_PIN, LOW);

    if (this_node == 200) {
      // if the Node # is 200 (which is also 0), blink POWER LED every 1 second if signal exists
      digitalWrite(POWER_PIN, HIGH);
      delay(1000);
      digitalWrite(POWER_PIN, LOW);
      delay(1000);
    } else {
      // if the Node # is a set number, trigger an LED accordingly
      if (currentPayload.program_1 == this_node || currentPayload.program_2 == this_node) {
        digitalWrite(PREVIEW_PIN, LOW);
        digitalWrite(PROGRAM_PIN, HIGH);
      } else if (currentPayload.preview == this_node) {
        digitalWrite(PROGRAM_PIN, LOW);
        digitalWrite(PREVIEW_PIN, HIGH);
      }
    }
  }
  
	// turn off LEDs when no radio signal exists (the past 5,5 second)
  if (millis() - last_radio_recv > 5500) {
    digitalWrite(PREVIEW_PIN, 0);
    digitalWrite(PROGRAM_PIN, 0);
    digitalWrite(POWER_PIN, 0);
  }
}

// reads the DIP switches and determines the Node #
void setNodeID() {

  // create an array of DIP pins
  const short numPins = 4;
  const int dipPins[] = {DIP1_PIN, DIP2_PIN, DIP3_PIN, DIP4_PIN};
  
  float j=0;
  
  for(int i=0; i < numPins; i++) {
    // Invert because of pullup resistor.
    if (!digitalRead(dipPins[i])) {
      j += pow(2, i);
    }
  }

  this_node = (int)(j + 0.5);

  // if the DIP switches are all OFF, assign 200 (alias to 0) to the Node #
  this_node = this_node == 0 ? 200 : this_node;
}

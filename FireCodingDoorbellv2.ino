// Connections
// Arduino:
//                                Serial MP3 Player Module (OPEN-SMART) / MOSFET Relay / Push-button
// D8 --------------------------- TX
// D7 --------------------------- RX
// D2 --------------------------- Output from the MOSFET relay. This is meant to be connected to negative terminal of load; i.e. it swings low when triggered. Thus the logic has to be inverted. Pin has to be pulled up by software.
// D3 --------------------------- Push-button to connect to ground. Logic is inverted. Pin has to be pulled up by software. 

// Include required libraries:
#include <SoftwareSerial.h>

// Define the RX and TX pins to establish UART communication with the MP3 Player Module.
#define MP3_RX 8 // to TX (this means D8 is set as RX pin on nano board, connected to TX of MP3)
#define MP3_TX 7 // to RX (this means D7 is set as TX pin on nano board, connected to RX of MP3)

// Define the input pins.
#define triggerPin 2 // note: to confirm if "2" means "D2"
#define volumePin 3 // note: to confirm if "3" means "D3"

// Designate the volume levels.
const int8_t high = 30;
const int8_t medium = 20;
const int8_t low = 10;
const int8_t mute = 0;

// Declare the variables.
int volumeState = 2; // 0 for mute, 1 for low, 2 for medium, 3 for high
bool triggerState = false;
bool changeVolState = false;
int8_t mp3_status[9];
unsigned long last_played = 0;
unsigned long last_changed = 0;

/*
Define the required MP3 Player Commands.
int8_t means 8-bit long new ('t') data type, i.e. a byte, the [] means it is an array of bytes
First byte is always 0x7e, last byte is always 0xef
Second byte states the number of bytes in between
*/

// Select storage device to TF card
static int8_t select_SD_card[] = {0x7e, 0x03, 0X35, 0x01, 0xef}; // 7E 03 35 01 EF

// Play with index: /01/001xxx.mp3
static int8_t play_first_song[] = {0x7e, 0x04, 0x41, 0x00, 0x01, 0xef}; // 7E 04 41 00 01 EF

// Play with index: /01/002xxx.mp3
static int8_t play_second_song[] = {0x7e, 0x04, 0x41, 0x00, 0x02, 0xef}; // 7E 04 41 00 02 EF

// Play the song.
static int8_t play[] = {0x7e, 0x02, 0x01, 0xef}; // 7E 02 01 EF

// Pause the song.
static int8_t pause[] = {0x7e, 0x02, 0x02, 0xef}; // 7E 02 02 EF

// Set the volume to high
static int8_t set_vol_high[] = {0x7E, 0x03, 0x31, high, 0xEF}; // 7E 03 31 high EF

// Set the volume to medium
static int8_t set_vol_medium[] = {0x7E, 0x03, 0x31, medium, 0xEF}; // 7E 03 31 medium EF

// Set the volume to low
static int8_t set_vol_low[] = {0x7E, 0x03, 0x31, low, 0xEF}; // 7E 03 31 low EF

// Set the volume to mute
static int8_t set_vol_mute[] = {0x7E, 0x03, 0x31, mute, 0xEF}; // 7E 03 31 mute EF

// Get status
static int8_t get_status[] = {0x7E, 0x02, 0x10, 0xEF}; //7E 02 10 EF


// Define the Serial MP3 Player Module.
SoftwareSerial MP3(MP3_RX, MP3_TX);


void setup() {

  // initialize the trigger pin as an input that is pulled high:
  pinMode(triggerPin, INPUT_PULLUP);

  // initialize the volume pin as an input:
  pinMode(volumePin, INPUT_PULLUP);

  // Initiate the serial monitor.
  Serial.begin(9600);
  
  // Initiate the Serial MP3 Player Module.
  MP3.begin(9600);

  // Initialise the volume to medium.
  send_command_to_MP3_player(set_vol_medium, 5);
  volumeState = 2;
  
  // Select the SD Card.
  send_command_to_MP3_player(select_SD_card, 5);
}

void loop() {
  
  // Play the chime if bell is pressed and more than 0.5 seconds elapsed before the last playing, otherwise do nothing.
  triggerState = !digitalRead(triggerPin);
  if ((triggerState == true) and ((millis()-last_played)) > 500) {
    
    // Go ahead to play if the player is not already playing, i.e. at "stopped" status.
    if (send_command_to_MP3_player(get_status, 4) == 0) {
      send_command_to_MP3_player(play_first_song, 6); 
      last_played = millis();
    }
    
  }
  
  // Change the volume if volume is pressed and more than 0.5 second elapsed before the last volume change, then play chime, otherwise do nothing.
  changeVolState = !digitalRead(volumePin);
  if ((changeVolState == true) and ((millis()-last_changed) > 500)) {
    change_volume();
    last_changed = millis();
  }

  // Clear the serial buffer if data comes in as we are not expecting to read any data. When data is expected by another function, the first intended byte can then be read.
  while (MP3.available() > 0) {
    MP3.read();
  }

}

int8_t send_command_to_MP3_player(int8_t command[], int len) { // first input is the command array itself, second input is the length of the command in number of bytes
  Serial.print("\nMP3 Command => ");
  for (int i = 0; i < len; i++) {
    MP3.write(command[i]);
    Serial.print(command[i], HEX);
  }

  //This is to wait till data arrives before reading.
  while (!(MP3.available() > 0)) {
  }

  //Read the series of bytes if the MP3 player returns its status.
  Serial.print("\nMP3 Status => ");
  for (int i = 0; i < 9; i++) {
    mp3_status[i] = MP3.read();
    Serial.print(mp3_status[i], HEX);
  }

  // The 8th byte should be HEX '0' if the player's status is 'stopped'.
  return mp3_status[7];

}

void change_volume() {
  switch (volumeState) {
    case 2:
      send_command_to_MP3_player(set_vol_high, 5);
      volumeState = 3;
      delay(100); // pause to allow MP3 player processing time before next command can be taken in
      send_command_to_MP3_player(play_first_song, 6);
      last_played = millis();
      break;
    case 3:
      send_command_to_MP3_player(set_vol_mute, 5);
      volumeState = 0;
      delay(100); // pause to allow MP3 player processing time before next command can be taken in
      send_command_to_MP3_player(play_first_song, 6);
      last_played = millis();
      break;
    case 0:
      send_command_to_MP3_player(set_vol_low, 5);
      volumeState = 1;
      delay(100); // pause to allow MP3 player processing time before next command can be taken in
      send_command_to_MP3_player(play_first_song, 6);
      last_played = millis();
      break;
    case 1:
      send_command_to_MP3_player(set_vol_medium, 5);
      volumeState = 2;
      delay(100); // pause to allow MP3 player processing time before next command can be taken in
      send_command_to_MP3_player(play_first_song, 6);
      last_played = millis();
      break;
  }
}

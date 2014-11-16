/*
 Camera Trigger
 
 Triggers camera using remote cord port by pulling output low.
 OLED - Output frame count
 Keypad - Set Speed
 	 
 */

#include <SD.h>
#include <TinyGPS_UBX.h>
#include <Keypad.h>
#include <U8glib.h>

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

byte rowPins[ROWS] = {8, 3, 4, 6}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 9, 5}; // connect to the column pinouts of the keypad

Keypad keypad = Keypad( makeKeymap(keys), rowPins, colPins, ROWS, COLS );


U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);	// Display which does not send AC

// On the Ethernet Shield, CS is pin 4. Note that even if it's not
// used as the CS pin, the hardware CS pin (10 on most Arduino boards,
// 53 on the Mega) must be left as an output or the SD library
// functions will not work.
const int chipSelect = 10;
File sdfile;

// constants won't change. Used here to 
// set pin numbers:
const int cameraPin = 2;   //Camera toggling pin
int frameCount = 0;        // Keep track of frames triggered

// Variables will change:
boolean lockState = false;               // state of GPS lock
unsigned long prevlockMillis = 0;        // will store last time lock state was updated

static char filename[13];
static bool fileOpened = false;

//Write buffer
#define BUFFSIZE 512
char buffer[BUFFSIZE];
char c;
int bufferidx;

// TinyGPS
TinyGPS gps;

unsigned long gps_time,gps_fix_age;
unsigned int loss_counter;

// no need to store these in the RAM anyway
static char str_buffer[25];
prog_char GPSstr_poll[] PROGMEM = "$PUBX,00*33";
prog_char GPSstr_setup1[] PROGMEM = "$PUBX,40,ZDA,0,0,0,0*44";
prog_char GPSstr_setup2[] PROGMEM = "$PUBX,40,GLL,0,0,0,0*5C";
prog_char GPSstr_setup3[] PROGMEM = "$PUBX,40,VTG,0,0,0,0*5E";
prog_char GPSstr_setup4[] PROGMEM = "$PUBX,40,GSV,0,0,0,0*59";
prog_char GPSstr_setup5[] PROGMEM = "$PUBX,40,GSA,0,0,0,0*4E";
prog_char GPSstr_setup6[] PROGMEM = "$PUBX,40,GGA,0,0,0,0*5A";
prog_char GPSstr_setup7[] PROGMEM = "$PUBX,40,RMC,0,0,0,0*47";
PROGMEM const char *str_table[] = {
  GPSstr_poll, GPSstr_setup1, GPSstr_setup2, GPSstr_setup3, 
  GPSstr_setup4, GPSstr_setup5, GPSstr_setup6, GPSstr_setup7
};

//Camera triggering settings
unsigned long prevtriggerMillis = 0;     // will store last time camera was fired
boolean triggering = false;
int h_cam_delay = 500; // half of camera delay used for creating edge trigger

void setup()
{
  GPS_setup();
  
  Screen_setup();
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  //Set Camera pin to output mode and HIGH
  pinMode(cameraPin,OUTPUT);
  digitalWrite(cameraPin,HIGH);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    // don't do anything more:
    return;
  }
  fileOpened = openSDFile();
  loss_counter = 0;
  
  keypad.addEventListener(keypadEvent); // Add an event listener for this keypad
  
  delay(3000);
}

void loop()
{
  /*
  GPS_poll();
  
  if(gps.has_fix())
  {
    gps.get_time(&gps_time,&gps_fix_age);
    // set the LED with the ledState of the variable:
    
    char time[40];
    sprintf(time, "%lu:%lu\n", gps_time,gps_fix_age);
    
    // dump time to file on SD card
    if (fileOpened) {
        sdfile.write(time);
        sdfile.flush();
    }
    else {
      //Wait for card
      fileOpened = openSDFile();
      delay(2000);
    }
    
    loss_counter = 1000;
  }
  else
  {
    if(loss_counter==0) lockState = false;
    else loss_counter--;
  }
  
  unsigned long lockMillis = millis();
 
  if(lockMillis - prevlockMillis > loss_counter) {
    // save the last time you blinked the LED 
    prevlockMillis = lockMillis;   

    // if the LED is off turn it on and vice-versa:
    lockState = false;
  }
  */
  //Draw on screen 
  // picture loop
  u8g.firstPage();  
  do {
    draw();
  } while( u8g.nextPage() );
  
  // Scan for keys
  char key = keypad.getKey();
  
  if (key){
    Serial.println(key);
  }
  
  if(triggering) {
    unsigned long trigMillis = millis();
    if(trigMillis-prevtriggerMillis > h_cam_delay)
    {
      digitalWrite(cameraPin,!digitalRead(cameraPin));
      prevtriggerMillis = trigMillis;
      //When camera pin goes low frame is triggered
      if(!digitalRead(cameraPin)) frameCount++;
    }
    
  }
}

bool openSDFile()
{
    // open log file
    for (unsigned int index = 0; index < 65535; index++) {
        char filename[16];
        sprintf(filename, "GPS%05d.TXT", index);
        if (!SD.exists(filename)) {
            sdfile = SD.open(filename, FILE_WRITE);
            if (sdfile) {
                return true;
            }
            break;
        }
    }
    return false;
}

void Screen_setup() {
  // assign default color value
  if ( u8g.getMode() == U8G_MODE_R3G3B2 ) {
    u8g.setColorIndex(255);     // white
  }
  else if ( u8g.getMode() == U8G_MODE_GRAY2BIT ) {
    u8g.setColorIndex(3);         // max intensity
  }
  else if ( u8g.getMode() == U8G_MODE_BW ) {
    u8g.setColorIndex(1);         // pixel on
  }
  else if ( u8g.getMode() == U8G_MODE_HICOLOR ) {
    u8g.setHiColorByRGB(255,255,255);
  }
}

void GPS_setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);
  // switch baudrate to 4800 bps
  //GPS_Serial.println("$PUBX,41,1,0007,0003,4800,0*13"); 
  //GPS_Serial.begin(4800);
  //GPS_Serial.flush();
  
  delay(500);
  
  // turn off all NMEA sentences for the uBlox GPS module
  // ZDA, GLL, VTG, GSV, GSA, GGA, RMC
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[1])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[2])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[3])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[4])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[5])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[6])));
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[7])));

  delay(500);
}

// request uBlox to give fresh data
boolean GPS_poll() {
  //GPS_Serial.println("$PUBX,00*33");
  Serial.println(strcpy_P(str_buffer, (char*)pgm_read_word(&str_table[0])));
  //delay(1000);
  unsigned long starttime = millis();
  while (true) {
    if (Serial.available()) {
      c = Serial.read();
      buffered_write();
      if (gps.encode(c))
        return true;
    }
    // that's it, can't wait any longer
    // i have short attention span..
    if (millis() - starttime > 1000) {
      break;
    }
  }
  return false;
}

void buffered_write()
{
  buffer[bufferidx++] = c;  // Store character in array and increment index

    // CR record termination or buffer full? (CR+LF is common, so don't check both)
    // See notes section regarding this implementation:
    if (c == '\r' || (bufferidx >= BUFFSIZE-1)) {
      buffer[bufferidx] = 0; // terminate it with null at current index
      sdfile.write((uint8_t *) buffer, (bufferidx + 1)); //write the program buffer to SD lib backend buffer
      sdfile.flush();
      bufferidx = 0;     // reset buffer pointer
    }
}

// Taking care of some special events.
void keypadEvent(KeypadEvent key){
    switch (keypad.getState()){
    case PRESSED:
        if (key == '#') {
            triggering = false;
        }
        if (key == '*') {
            triggering = true;
        }
        if (key == '0') {
          h_cam_delay = 500;
        }
        if (key == '1') {
          h_cam_delay = 550;
        }
        if (key == '2') {
          h_cam_delay = 600;
        }
        if (key == '3') {
          h_cam_delay = 650;
        }
        if (key == '4') {
          h_cam_delay = 700;
        }
        if (key == '5') {
          h_cam_delay = 750;
        }
        if (key == '6') {
          h_cam_delay = 800;
        }
        if (key == '7') {
          h_cam_delay = 850;
        }
        if (key == '8') {
          h_cam_delay = 900;
        }
        if (key == '9') {
          h_cam_delay = 950;
        }
        break;

    case RELEASED:
        break;

    case HOLD:
        break;
    }
}

void draw(void) {
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  char buf[12];
  u8g.drawStr( 0, 10, F("Aero Trigger"));
  u8g.drawStr( 0, 30, F("Speed"));
  u8g.drawStr( 50, 30,itoa(h_cam_delay*2,buf,10));
  u8g.drawStr( 0, 50, F("Frames"));
  u8g.drawStr( 50, 50,itoa(h_cam_delay*2,buf,10));
}

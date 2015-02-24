/*
 Camera Trigger
 
 Triggers camera using remote cord port by pulling output low.
 OLED - Output frame count
 Keypad - Set Speed
 	 
 */
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

// constants won't change. Used here to 
// set pin numbers:
const int cameraPin = 2;   //Camera toggling pin
int frameCount = 0;        // Keep track of frames triggered


//Camera triggering settings
unsigned long prevtriggerMillis = 0;     // will store last time camera was fired
boolean triggering = false;
int h_cam_delay = 450; // half of camera delay used for creating edge trigger

void setup()
{
  
  Screen_setup();
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  //Set Camera pin to output mode and HIGH
  pinMode(cameraPin,OUTPUT);
  digitalWrite(cameraPin,HIGH);
  
  keypad.addEventListener(keypadEvent); // Add an event listener for this keypad
  
  delay(3000);
}

void loop()
{
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
          h_cam_delay = 450;
        }
        if (key == '1') {
          h_cam_delay = 500;
        }
        if (key == '2') {
          h_cam_delay = 550;
        }
        if (key == '3') {
          h_cam_delay = 600;
        }
        if (key == '4') {
          h_cam_delay = 650;
        }
        if (key == '5') {
          h_cam_delay = 700;
        }
        if (key == '6') {
          h_cam_delay = 750;
        }
        if (key == '7') {
          h_cam_delay = 800;
        }
        if (key == '8') {
          h_cam_delay = 850;
        }
        if (key == '9') {
          h_cam_delay = 900;
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
  u8g.drawStr( 50, 30,itoa(h_cam_delay*2+100,buf,10));
  u8g.drawStr( 0, 50, F("Frames"));
  u8g.drawStr( 50, 50,itoa(frameCount,buf,10));
}

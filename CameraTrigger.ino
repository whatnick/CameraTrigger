/*
 Camera Trigger
 
 Triggers camera using remote cord port by pulling output low.
 OLED - Output frame count (https://github.com/olikraus/u8glib)
 Keypad - Set Speed (https://github.com/whatnick/i2ckeypad)
 TimeOne - Accurate timing of pulses (https://github.com/PaulStoffregen/TimerOne)
 	 
 */
#include <U8glib.h>
#include <TimerOne.h>

// With A0, A1 and A2 of PCF8574 to ground I2C address is 0x20
#include <Keypad_I2C.h>
#include <Keypad.h>
#define I2CADDR 0x20

const byte ROWS = 4; //four rows
const byte COLS = 3; //three columns
char keys[ROWS][COLS] = {
  {'1','2','3'},
  {'4','5','6'},
  {'7','8','9'},
  {'*','0','#'}
};

// Digitran keypad, bit numbers of PCF8574 i/o port
byte rowPins[ROWS] = {5, 0, 1, 3}; // connect to the row pinouts of the keypad
byte colPins[COLS] = {4, 6, 2}; // connect to the column pinouts of the keypad

Keypad_I2C kpd( makeKeymap(keys), rowPins, colPins, ROWS, COLS, I2CADDR, PCF8574 );


U8GLIB_SSD1306_128X64 u8g(U8G_I2C_OPT_NO_ACK);	// Display which does not send AC

// constants won't change. Used here to 
// set pin numbers:
const int cameraPin = 9;   //Camera toggling pin
int camState = LOW; //Optocoupler turns on HIGH and fires camera

//Changing frame counter set by interrupt
volatile unsigned long frameCount = 0;        // Keep track of frames triggered
//Camera triggering settings
volatile boolean triggering = false;
volatile float cam_delay = 0.9f; // half of camera delay used for creating edge trigger

//Live battery voltage monitor
volatile float voltage;
const char CH_status_print[][4]=
{
  "bat","chg","ext","err"
};
volatile unsigned char CHstatus;
unsigned long previousMillis = 0;        // will store last time battery was checked
const long interval = 1000;

bool redraw = true;

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

void setup()
{
  Serial.begin(57600);
  analogReference(INTERNAL); 
  Screen_setup();
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  //Set Camera pin to output mode and LOW
  pinMode(cameraPin,OUTPUT);
  digitalWrite(cameraPin,LOW);
  
  Timer1.initialize(cam_delay*50000l);
  Timer1.attachInterrupt(triggerCam);
  
  kpd.begin();
  //Initialize Battery values
  Bat_init();
}

void loop()
{
  int BatteryValue;
  
  //Draw on screen 
  // picture loop
  if (redraw){
    u8g.firstPage();  
    do {
      draw();
    } while( u8g.nextPage() );
    redraw = false;  //Clear flag to redraw
  }
  // Scan for keys
  char key = kpd.getKey();
  
  if (key){
    keypadEvent(key);
    redraw=true;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    //Save the battery update time
    previousMillis = currentMillis;
    BatteryValue = analogRead(A7);
    voltage = BatteryValue * (1.1 / 1024)* (10+2)/2;  //Voltage devider
    CHstatus = read_charge_status();//read the charge status
    redraw = true;
  }
}

void triggerCam()
{
  if(triggering) {
    if (camState == LOW) {
      camState = HIGH;
      //Optocoupler triggers camera on HIGH
      frameCount = frameCount + 1;  // increase camera frame counter
    } else {
      camState = LOW;
    }
    digitalWrite(cameraPin, camState);
    //Redraw screen
    redraw = true;
  }
}

void Bat_init()
{
  int BatteryValue = analogRead(A7);
  voltage = BatteryValue * (1.1 / 1024)* (10+2)/2;  //Voltage devider
  CHstatus = read_charge_status();//read the charge status
}


// Taking care of some special events.
void keypadEvent(char key){
      if (key == '#') {
          noInterrupts();
          triggering = false;
          interrupts();
          return;
      }
      if (key == '*') {
          noInterrupts();
          triggering = true;
          interrupts();
          return;
      }

      //If already triggering ignore numbers
      if(triggering) return;
      
      if(cam_delay < 1.0f) cam_delay = 0.0f;

      switch(key) 
      {
        case '0': {
          cam_delay = cam_delay*10;
          break;
        }
        case '1': {
          cam_delay = (cam_delay*10)+1;
          break;
        }
        case '2': {
          cam_delay = (cam_delay*10)+2;
          break;
        }
        case '3': {
          cam_delay = (cam_delay*10)+3;
          break;
        }
        case '4': {
          cam_delay = (cam_delay*10)+4;
          break;
        }
        case '5': {
          cam_delay = (cam_delay*10)+5;
          break;
        }
        case '6': {
          cam_delay = (cam_delay*10)+6;
          break;
        }
        case '7': {
          cam_delay = (cam_delay*10)+7;
          break;
        }
        case '8': {
          cam_delay = (cam_delay*10)+8;
          break;
        }
        case '9': {
          cam_delay = (cam_delay*10)+9;
          break;
        }
      }
      
      cam_delay = (int)(cam_delay)%100;

      //Avoid setting cam_delay to 0.0f
      if(cam_delay == 0) 
      {
        cam_delay = 0.9f;
        return;
      }
      
      Timer1.setPeriod(cam_delay*50000l);
}

unsigned char read_charge_status(void)
{
  unsigned char CH_Status=0;
  unsigned int ADC6=analogRead(6);
  if(ADC6>900)
  {
    CH_Status = 0;//sleeping
  }
  else if(ADC6>550)
  {
    CH_Status = 1;//charging
  }
  else if(ADC6>350)
  {
    CH_Status = 2;//done
  }
  else
  {
    CH_Status = 3;//error
  }
  return CH_Status;
}

void draw(void) {
  unsigned long frameCopy;
  float delayCopy;
  float voltCopy;
  char statCopy;
  
  noInterrupts();
  frameCopy = frameCount;
  delayCopy = cam_delay/10.0f;
  voltCopy = voltage;
  statCopy = CHstatus;
  interrupts();
  // graphic commands to redraw the complete screen should be placed here  
  u8g.setFont(u8g_font_unifont);
  //u8g.setFont(u8g_font_osb21);
  char buf[20];
  u8g.drawStr( 0, 10, F("Aero Trigger"));
  u8g.drawStr( 0, 30, F("Delay:"));
  if(cam_delay<1.0f)
  {
    u8g.drawStr( 50, 30,F("NOT SET"));
  }
  else
  {
    u8g.setPrintPos(50,30);
    u8g.print(delayCopy,1);
    u8g.drawStr( 75, 30,F("s"));
  }
  u8g.drawStr( 0, 45, F("Frames:"));
  u8g.drawStr( 60, 45,itoa(frameCopy,buf,10));

  u8g.drawStr( 0, 60, F("Bat:"));
  u8g.setPrintPos(35,60);
  u8g.print(voltCopy,1);
  u8g.drawStr( 60, 60,F("V"));
  u8g.drawStr( 80, 60, CH_status_print[statCopy]);
}

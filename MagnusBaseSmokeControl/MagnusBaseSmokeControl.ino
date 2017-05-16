#include <Adafruit_NeoPixel.h>
#include <stdlib.h>

// BASE ARDUINO
//
// this one cotrols the smoke, and the base LEDs
//

// first communication pin for neo pixel string
#define BASE_PIN 2

#define SMOKE1_PIN  12
#define SMOKE2_PIN  11

#define SMOKE_ON  LOW
#define SMOKE_OFF HIGH

#define SMOKE_TIMER   10000 // 10 seconds
#define SMOKE_MINIMUM 2000  // 2 seconds
#define SMOKE_MAXIMUM 8000  // 8 seconds

int8_t  smoke1_flag = 0;
int8_t  smoke2_flag = 0;
uint16_t smoke1_timer;
uint16_t smoke2_timer;

#define STRIP_TIME_COUNT  100 // ms

enum Direction { EAST = 0, NORTHEAST, NORTH, NORTHWEST, WEST, SOUTHWEST, SOUTH, SOUTHEAST };
enum Ownership {  neutral = 0, enlightened, resistance };

struct pixel_string {
  uint16_t phase;
  uint32_t timing;
};

#define NUM_STRINGS 1 // just one for all resonators, but 12 LEDs per resonator in this string

#define NUM_RESONATORS          1 // 8
#define NUM_LEDS_PER_RESONATOR  12
#define NUM_LEDS  (NUM_RESONATORS * NUM_LEDS_PER_RESONATOR)   //96

pixel_string strings[NUM_STRINGS];

#define STRING_TYPE NEO_RGB // NEO_RGBW // 

// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUM_LEDS, BASE_PIN, STRING_TYPE + NEO_KHZ800);


// Serial I/O
int8_t command[32];
int8_t in_index = 0;

void start_serial(void)
{
  in_index = 0;
  Serial.begin(9600);
}

bool collect_serial(void)
{
  while( Serial.available() > 0 )
  {
    int8_t ch = Serial.read();
    if( ch == '\n' ) 
    {
      command[in_index] = 0; // terminate
      if( in_index > 0 )
      {
        in_index = 0;
        return true;
      }
    }
    else if( ch == 0x0a || ch == 0x13 ) // carriage return or linefeed (ignore
    {
    }
    else
    {
      command[in_index] = ch;
      in_index++;
      //if( in_index >= 32 ) //sizeof(command) )
      //  in_index = 0;
    }
  }
  return false;
}

uint8_t getPercent(uint8_t *buffer)
{
  unsigned long inVal = strtoul(buffer,NULL,10);
  return uint8_t( constrain(inVal, 0, 100) );
}


uint8_t dir;
uint8_t percent;
uint8_t owner;    // 0=neutral, 1=enl, 2=res

uint32_t last_millis;

// the setup function runs once when you press reset or power the board
void setup() 
{
  start_serial();
  strip.begin();
  uint8_t i;
  for( i = 0; i<NUM_STRINGS; i++)
  {
    strip.setPin(i+BASE_PIN);
    strip.show(); // Initialize all pixels to 'off'

    strings[i].phase = 0;
    strings[i].timing = 0;
  }

  dir = EAST; // begin on the north resonator
  owner = neutral;
  percent = 0;
  smoke1_flag = 0;
  smoke1_timer = 0;
  smoke2_flag = 0;
  smoke2_timer = SMOKE_TIMER/2; // alternate cycles
  pinMode(SMOKE1_PIN, OUTPUT);
  digitalWrite(SMOKE1_PIN, SMOKE_OFF);
  pinMode(SMOKE2_PIN, OUTPUT);
  digitalWrite(SMOKE2_PIN, SMOKE_OFF);

  last_millis = millis();
}

// the loop function runs over and over again forever
void loop() 
{
  uint16_t i, val;

  if( collect_serial() )
  {
    // we have valid buffer of serial input
    if( command[0] == 'e' || command[0] == 'E' )
    {
      owner = enlightened;
      percent = getPercent(&command[1]);
    }
    else if( command[0] == 'r' || command[0] == 'R' )
    {
      owner = resistance;
      percent = getPercent(&command[1]);
    }
    else if( command[0] == 'n' || command[0] == 'N' )
    {
      owner = neutral;
      percent = 100;
    }
    //Serial.print((char *)command); Serial.print(" - ");Serial.print(command[0],DEC);Serial.print(": "); 
    //Serial.print("owner "); Serial.print(owner,DEC); Serial.print(", percent "); Serial.println(percent,DEC); 
  }

  if(strings[dir].timing < millis() )
  {
    strings[dir].timing += STRIP_TIME_COUNT; // every 100 milliseconds we will check this direction
    
    strip.setPin(dir+BASE_PIN);  // pick the string
  
    uint8_t red, green, blue, white;
    red = 0x20; green = 0; blue = 0x20; white = 0x20;
    if( owner == neutral ) 
    {
      red = 0; green = 0; blue = 0; white = 0x40;
    }
    else if( owner == resistance )
    {
      red = 0; green = 0x1f; blue = 0xff;; white = 0;
    }
    else if( owner == enlightened )
    {
      red = 0x1f; green = 0xff; blue = 0; white = 0;
    }
  
    for(i=0; i < strip.numPixels(); i++)
    {
        //strip.setPixelColor(i, green,blue,red); // for RGB order is funny?
        strip.setPixelColor(i, green, red, blue,white);
        strip.setBrightness((uint8_t)((uint16_t)(255*percent)/100));
    }    
    strip.show();
  
    dir++;
    if( dir >= NUM_STRINGS )
      dir = 0;
  }


  // smoke on/off
  if( millis() != last_millis )
  {
    last_millis = millis();

    if( ++smoke1_timer > SMOKE_MINIMUM )
    {
      if( smoke1_flag )
      {
        Serial.println("smoke 1 OFF"); 
        digitalWrite(SMOKE1_PIN, SMOKE_OFF);
        smoke1_flag = 0;
      }
      if( smoke1_timer > SMOKE_TIMER )
      {
        Serial.println("smoke 1 ON"); 
        smoke1_timer = 0;
        digitalWrite(SMOKE1_PIN, SMOKE_ON);
        smoke1_flag = 1;
      }
    }
    if( ++smoke2_timer > SMOKE_MINIMUM )
    {
      if( smoke2_flag )
      {
        Serial.println("smoke 2 OFF"); 
        digitalWrite(SMOKE2_PIN, SMOKE_OFF);
        smoke2_flag = 0;
      }
      if( smoke2_timer > SMOKE_TIMER )
      {
        Serial.println("smoke 2 ON"); 
        smoke2_timer = 0;
        digitalWrite(SMOKE2_PIN, SMOKE_ON);
        smoke2_flag = 1;
      }
    }
  }

}



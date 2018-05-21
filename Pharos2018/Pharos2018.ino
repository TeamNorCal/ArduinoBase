#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
  #include <avr/power.h>
#endif

// ARDUINO Smoke, Airhorn and PHAROS sign LEDs control
//
// this one cotrols the smoke, the airhorn and the PHAROS sign LEDs
//
// LED string is on Digital 2
// Relay 1    is on Digital 11
// Relay 2    is on Digital 12
// Relay 3    is on Digital 10

// smoke on the water, fire in the sky

#define SMOKE1_PIN  12
#define SMOKE2_PIN  11
#define AIRHORN_PIN 10
#define ONBOARD_LED 13

#define SMOKE_CYCLE   10000 // 10 second cycles

#define SMOKE1_START   1000 // 1 second
#define SMOKE1_END     4000  // 4 second

#define SMOKE2_START   6000  // 6 second
#define SMOKE2_END     9000  // 9 second

#define AIRHORN_CYCLE (5 * 60000) // 5 minute cycles
#define AIRHORN_START 10000 // 5 second
#define AIRHORN_END   15000 // 10 second

unsigned long smoke_timer;
unsigned long airhorn_timer;

#define PIN 2

Adafruit_NeoPixel strip = Adafruit_NeoPixel(173, PIN, NEO_GRB + NEO_KHZ800);
// PHAROS sign has 173 LEDs

void setup() {
  //Serial.begin(9600);
  strip.begin();
  strip.show(); // Initialize all pixels to 'off'

  pinMode(SMOKE1_PIN, OUTPUT);
  pinMode(SMOKE2_PIN, OUTPUT);
  pinMode(AIRHORN_PIN, OUTPUT);
  pinMode(ONBOARD_LED, OUTPUT);
}


void loop() 
{
  smoke_timer = millis() % SMOKE_CYCLE;
  airhorn_timer = millis() % AIRHORN_CYCLE; 

//  digitalWrite(AIRHORN_LED, (airhorn_timer < 5000);
  if (((airhorn_timer) % 1000) == 0) {
    // Serial.println(airhorn_timer);
  }
  digitalWrite(ONBOARD_LED, (airhorn_timer > AIRHORN_START) and (airhorn_timer < AIRHORN_END));  // use for lighting onboard LED
  digitalWrite(AIRHORN_PIN, (airhorn_timer > AIRHORN_START) and (airhorn_timer < AIRHORN_END));
  digitalWrite(SMOKE1_PIN, (smoke_timer > SMOKE1_START) and (smoke_timer < SMOKE1_END));
  digitalWrite(SMOKE2_PIN, (smoke_timer > SMOKE2_START) and (smoke_timer < SMOKE2_END));
  
// Neopixel animationloop code

// Some example procedures showing how to display to the pixels:
//  colorWipe(strip.Color(255, 0, 0), 50); // Red
//  colorWipe(strip.Color(0, 255, 0), 50); // Green
//  colorWipe(strip.Color(0, 127, 0), 50); // Blue
//  colorWipe(strip.Color(0, 0, 0, 255), 50); // White RGBW
//Send a theater pixel chase in...
//  theaterChase(strip.Color(0, 255, 0), 500); // White
//  theaterChase(strip.Color(127, 0, 0), 50); // Red
//  theaterChase(strip.Color(0, 0, 127), 50); // Blue
//  rainbow_window(5);
 // rainbowCycle(20);    <====== this one
//  theaterChaseRainbow(50);
}

// Neopixel Code 

// Fill the dots one after the other with a color
void colorWipe(uint32_t c, uint8_t wait) {
  for(uint16_t i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, c);
    strip.show();
    delay(wait);
  }
}

void rainbow(uint8_t wait) {
  uint16_t i, j, k;

  j = (millis() / 5000) % 256; // change colors every 5 seconds or so?
  for(i=0; i<strip.numPixels(); i++) {
    // strip.setPixelColor(i, Wheel((i+j) & 255));
    for(k=0; k<25; k++) {    //25
      strip.setPixelColor(k, Wheel((j) & 255));
    }
    for(k=26; k<56; k++) {
      strip.setPixelColor(k, Wheel((j+42) & 255));
    }
    for(k=57; k<85; k++) {
      strip.setPixelColor(k, Wheel((j+84) & 255));
    }
    for(k=86; k<118; k++) {
      strip.setPixelColor(k, Wheel((j+126) & 255));
    }
    for(k=119; k<149; k++) {
      strip.setPixelColor(k, Wheel((j+172) & 255));
    }
    for(k=150; k<173; k++) {
      strip.setPixelColor(k, Wheel((j+216) & 255));
    }
  }
  strip.show();
}



void rainbow_old(uint8_t wait) {
  uint16_t i, j, k;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      // strip.setPixelColor(i, Wheel((i+j) & 255));
      for(k=0; k<25; k++) {    //25
        strip.setPixelColor(k, Wheel((j) & 255));
      }
      for(k=26; k<56; k++) {
        strip.setPixelColor(k, Wheel((j+42) & 255));
      }
      for(k=57; k<85; k++) {
        strip.setPixelColor(k, Wheel((j+84) & 255));
      }
      for(k=86; k<118; k++) {
        strip.setPixelColor(k, Wheel((j+126) & 255));
      }
      for(k=119; k<149; k++) {
        strip.setPixelColor(k, Wheel((j+172) & 255));
      }
      for(k=150; k<173; k++) {
        strip.setPixelColor(k, Wheel((j+216) & 255));
      }
    }
    strip.show();
    delay(wait);
  }
}

void rainbow_window(uint8_t wait) {
  uint16_t i, j, k;

  for(j=0; j<256; j++) {
    for(i=0; i<strip.numPixels(); i++) {
      // strip.setPixelColor(i, Wheel((i+j) & 255));
      for(k=0; k<4; k++) {    //25
        strip.setPixelColor(k, Wheel((j) & 255));
      }
      for(k=5; k<9; k++) {
        strip.setPixelColor(k, Wheel((j+42) & 255));
      }
      for(k=10; k<14; k++) {
        strip.setPixelColor(k, Wheel((j+84) & 255));
      }
      for(k=15; k<19; k++) {
        strip.setPixelColor(k, Wheel((j+126) & 255));
      }
      for(k=20; k<24; k++) {
        strip.setPixelColor(k, Wheel((j+172) & 255));
      }
      for(k=25; k<29; k++) {
        strip.setPixelColor(k, Wheel((j+216) & 255));
      }
    }
    strip.show();
    delay(wait);
  }
}


// Slightly different, this makes the rainbow equally distributed throughout
void rainbowCycle(uint8_t wait) {
  uint16_t i, j;

  for(j=0; j<256*5; j++) { // 5 cycles of all colors on wheel
    for(i=0; i< strip.numPixels(); i++) {
      strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
    }
    strip.show();
    delay(wait);
  }
}

//Theatre-style crawling lights.
void theaterChase(uint32_t c, uint8_t wait) {
  for (int j=0; j<10; j++) {  //do 10 cycles of chasing
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, c);    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

//Theatre-style crawling lights with rainbow effect
void theaterChaseRainbow(uint8_t wait) {
  for (int j=0; j < 256; j++) {     // cycle all 256 colors in the wheel
    for (int q=0; q < 3; q++) {
      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, Wheel( (i+j) % 255));    //turn every third pixel on
      }
      strip.show();

      delay(wait);

      for (uint16_t i=0; i < strip.numPixels(); i=i+3) {
        strip.setPixelColor(i+q, 0);        //turn every third pixel off
      }
    }
  }
}

// Input a value 0 to 255 to get a color value.
// The colours are a transition r - g - b - back to r.
uint32_t Wheel(byte WheelPos) {
  WheelPos = 255 - WheelPos;
  if(WheelPos < 85) {
    return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
  }
  if(WheelPos < 170) {
    WheelPos -= 85;
    return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
  }
  WheelPos -= 170;
  return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
}

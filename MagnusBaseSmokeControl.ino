#include <Adafruit_NeoPixel.h>
#include <stdlib.h>
#include <string.h>
#include "animation.hpp"
#include "circbuff.hpp"
// BASE ARDUINO
//
// this one cotrols the smoke, and the base LEDs
//
// LED string is on Digital 2
// Relay 1    is on Digital 11
// Relay 2    is on Digital 12

// Test setups.. I had a spare NeoPixel ring (16 pixels) to use for testing
// for that setup we use:  8 resos., 2 LEDs/reso and NEO_GRB pixels
// for the final set up we will use 8 resonators, 12 LEDs per resonator and RGBW
#define NUM_RESONATORS          1 //8
#define NUM_LEDS_PER_RESONATOR  12 //2 //12
const bool RGBW_SUPPORT = false;

const unsigned int NUM_STRINGS = NUM_RESONATORS; // just one for all resonators, but 12 LEDs per resonator in this string
const unsigned int QUEUE_SIZE = 3;

// Mask to clear 'upper case' ASCII bit
const char CASE_MASK = ~0x20;

const uint16_t LEDS_PER_STRAND =  NUM_LEDS_PER_RESONATOR; //(NUM_RESONATORS * NUM_LEDS_PER_RESONATOR);   //96

// first communication pin for neo pixel string
#define BASE_PIN 2

// smoke on the water, fire in the sky
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

#define STRIP_TIME_COUNT  10 // ms

enum Direction { EAST = 0, NORTHEAST, NORTH, NORTHWEST, WEST, SOUTHWEST, SOUTH, SOUTHEAST };
enum Ownership {  neutral = 0, enlightened, resistance };

// Static animation implementations singleton
Animations animations;

typedef CircularBuffer<Animation *, QUEUE_SIZE> QueueType;

AnimationState states[NUM_STRINGS][QUEUE_SIZE];
QueueType animationQueues[NUM_STRINGS];


// Parameter 1 = number of pixels in strip
// Parameter 2 = Arduino pin number (most are valid)
// Parameter 3 = pixel type flags, add together as needed:
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
Adafruit_NeoPixel strip = Adafruit_NeoPixel(LEDS_PER_STRAND, BASE_PIN, (RGBW_SUPPORT ? NEO_GRBW : NEO_GRB) + NEO_KHZ800);


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
uint32_t current_millis;
uint32_t smoke_millis;

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
  current_millis = last_millis;
  smoke_millis = current_millis;
}

// the loop function runs over and over again forever
void loop() 
{
  uint16_t i, val;

    if( collect_serial() )
    {
        // we have valid buffer of serial input
        char cmd = command[0];
//        if(strlen(cmd) == CONSTANT_LENGTH )
        switch (cmd) {
            Color c;
            case '*':
                Serial.println("Magnus Core Node");  // Magnus Base Node
                break;
            case 'E':
            case 'e':
            case 'R':
            case 'r':
                owner = (cmd & CASE_MASK) == 'E' ? enlightened : resistance;
                percent = getPercent(&command[1]);
                uint8_t red, green, blue, white;
                c = ToColor(0x00, 
                        owner == enlightened ? 0xff : 0x00,
                        owner == resistance ? 0xff : 0x00,
                        0x00);
                for (int i = 0; i < NUM_STRINGS; i++) {
                    QueueType& animationQueue = animationQueues[i];
                    animationQueue.setTo(&animations.pulse);
                    unsigned int stateIdx = animationQueue.lastIdx();
                    double initialPhase = ((double) i) / NUM_STRINGS;
                    animations.pulse.init(states[i][stateIdx], strip, c, initialPhase);
                }
                break;

            case 'N':
            case 'n':
                owner = neutral;
                percent = 20;
                if (RGBW_SUPPORT) {
                    c = ToColor(0x00, 0x00, 0x00, 0xff);
                } else {
                    c = ToColor(0xff, 0xff, 0xff);
                }
                for (int i = 0; i < NUM_STRINGS; i++) {
                    QueueType& animationQueue = animationQueues[i];
                    animationQueue.setTo(&animations.redFlash);
                    unsigned int stateIdx = animationQueue.lastIdx();
                    animations.redFlash.init(states[i][stateIdx], strip, RGBW_SUPPORT);
                    animationQueue.add(&animations.solid);
                    stateIdx = animationQueue.lastIdx();
                    animations.solid.init(states[i][stateIdx], strip, c);
                }
                break;

            default:
                Serial.print("Unkwown command "); Serial.println(cmd);
                break;
        }
        Serial.print((char *)command); Serial.print(" - ");Serial.print(command[0],DEC);Serial.print(": "); 
        Serial.print("owner "); Serial.print(owner,DEC); Serial.print(", percent "); Serial.println(percent,DEC); 
    }

  current_millis = millis();
  if(current_millis > last_millis + STRIP_TIME_COUNT )
  {
    last_millis = current_millis; // every X milliseconds we will check this direction
    
    strip.setPin(dir+BASE_PIN);  // pick the string
//    strip.setPin(BASE_PIN);  // pick the string

    QueueType& animationQueue = animationQueues[dir];
    unsigned int queueSize = animationQueue.size();
    if (queueSize > 1) {
        if (animationQueue.peek()->done(states[dir][animationQueue.currIdx()])) {
            animationQueue.remove();
            queueSize--;
            animationQueue.peek()->start(states[dir][animationQueue.currIdx()]);
        }
    }
    if (queueSize > 0) {
        //strip.setBrightness(255);
        strip.setBrightness((uint8_t)((uint16_t)(255*(percent/100.0))));
        //pCurrAnimation->doFrame(states[0], strip);
        animationQueue.peek()->doFrame(states[dir][animationQueue.currIdx()], strip);
    }

    // Update one strand each time through the loop
    dir = (dir + 1) % NUM_STRINGS;
  }


  // smoke on/off
  if( current_millis != smoke_millis )
  {
    smoke_millis = current_millis;

    if( ++smoke1_timer > SMOKE_MINIMUM )
    {
      if( smoke1_flag )
      {
        //Serial.println("smoke 1 OFF"); 
        digitalWrite(SMOKE1_PIN, SMOKE_OFF);
        smoke1_flag = 0;
      }
      if( smoke1_timer > SMOKE_TIMER )
      {
        //Serial.println("smoke 1 ON"); 
        smoke1_timer = 0;
        digitalWrite(SMOKE1_PIN, SMOKE_ON);
        smoke1_flag = 1;
      }
    }
    if( ++smoke2_timer > SMOKE_MINIMUM )
    {
      if( smoke2_flag )
      {
        //Serial.println("smoke 2 OFF"); 
        digitalWrite(SMOKE2_PIN, SMOKE_OFF);
        smoke2_flag = 0;
      }
      if( smoke2_timer > SMOKE_TIMER )
      {
        //Serial.println("smoke 2 ON"); 
        smoke2_timer = 0;
        digitalWrite(SMOKE2_PIN, SMOKE_ON);
        smoke2_flag = 1;
      }
    }
  }

}



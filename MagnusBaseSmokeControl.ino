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
// or 1 string, with 8x12 resonators
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

bool SmokeActive(int counter, int percent)
{
  // smoke is active from t=0 until our counter goes over this scaled time
  return counter > SMOKE_MINIMUM + ((SMOKE_MAXIMUM-SMOKE_MINIMUM) * percent / 100);
}

#define STRIP_TIME_COUNT  10 // ms
const unsigned long LED_UPDATE_PERIOD = 5; // in ms. Time between drawing a frame on _any_ LED strip.

//const long BAUD_RATE = 115200;
const long BAUD_RATE = 9600;

// Command format:
// Fnnnnnnnnp12345678mmmm\n
// F = faction: EeRrNn. Capitalized on state change; lowercase if same as before
// n = resonator level
// p = portal health percentage
// 1-8 = resonator health percentage
// m = mod status
//
// percentage health encoded as single character. Space (' ') is 0%, and then each ASCII character
// above it is 2% greater, up to 'R' for 100%
//
// Mods:
// ' ' - No mod present in this slot
// '0' - FA Force Amp
// '1' - HS-C Heat Shield, Common
// '2' - HS-R Heat Shield, Rare
// '3' - HS-VR Heat Shield, Very Rare
// '4' - LA-R Link Amplifier, Rare
// '5' - LA-VR Link Amplifier, Very Rare
// '6' - SBUL SoftBank Ultra Link
// '7' - MH-C MultiHack, Common
// '8' - MH-R MultiHack, Rare
// '9' - MH-VR MultiHack, Very Rare
// 'A' - PS-C Portal Shield, Common
// 'B' - PS-R Portal Shield, Rare
// 'C' - PS-VR Portal Sheild, Very Rare
// 'D' - AXA AXS Shield
// 'E' - T Turret

const unsigned int COMMAND_LENGTH = 22;
const unsigned int PORTAL_STRENGTH_INDEX = 9;

enum Direction { EAST = 0, NORTHEAST, NORTH, NORTHWEST, WEST, SOUTHWEST, SOUTH, SOUTHEAST };
enum Ownership {  neutral = 0, enlightened, resistance, initial };

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
const int ioSize = 64;
char command[32];
int8_t in_index = 0;

unsigned long nextUpdate;

void start_serial(void)
{
  in_index = 0;
  Serial.begin(BAUD_RATE);
}

enum SerialStatus { IDLE, IN_PROGRESS, COMMAND_COMPLETE };

uint8_t collect_serial(void)
{
    SerialStatus status = in_index == 0 ? IDLE : IN_PROGRESS;
    int avail = Serial.available();
    while (Serial.available() > 0)
    {
        int8_t ch = Serial.read();
        if ( ch == '\n' )
        {
            command[in_index] = 0; // terminate
            if ( in_index > 0 )
            {
                in_index = 0;
                return COMMAND_COMPLETE;
            }
        }
        else if (ch != '\r') // ignore CR
        {
            command[in_index] = ch;
            if ( in_index < (sizeof(command)/sizeof(command[0])) - 2 ) {
                in_index++;
            }
        }
    }
    return status;
}

uint8_t getPercent(uint8_t *buffer)
{
  unsigned long inVal = strtoul(buffer,NULL,10);
  return uint8_t( constrain(inVal, 0, 100) );
}

unsigned int decodePercent(const char encoded) {
    return (encoded - ' ') * 2;
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
  //owner = neutral;
  owner = initial;
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
  nextUpdate = last_millis;
  current_millis = last_millis;
  smoke_millis = last_millis;
}

// the loop function runs over and over again forever
void loop() 
{
  uint16_t i, val;
  uint32_t now = millis();

    SerialStatus status = collect_serial();
    if (status == COMMAND_COMPLETE)
    {
        // we have valid buffer of serial input
        char cmd = command[0];
        Ownership newOwner = owner;
        switch (cmd) {
            case '*':
                Serial.println("Magnus Base Node");
                break;
            case 'E':
            case 'e':
            case 'R':
            case 'r':
                int len;
                len = strlen(command);
                if (len != COMMAND_LENGTH) {
                    Serial.print("Invalid length of command \"");Serial.print(command);Serial.print("\": ");Serial.println(len, DEC);
                    break;
                }
                newOwner = (cmd & CASE_MASK) == 'E' ? enlightened : resistance;
                char strength;
                strength = command[PORTAL_STRENGTH_INDEX];
                percent = decodePercent(strength);
                //Serial.print("Owner ");Serial.print(newOwner, DEC);Serial.print(" pct ");Serial.println(percent, DEC);
                break;

            case 'N':
            case 'n':
                newOwner = neutral;
                percent = 0; // always
                //Serial.println("Neutral");
                break;

            default:
                Serial.print("Unknown command "); Serial.println(cmd);
                break;
        }

        // Check for ownership change
        if (newOwner != owner) {
            owner = newOwner;
            Color c;
            switch (owner) {
                case enlightened:
                case resistance:
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
                        animations.pulse.init(now, states[i][stateIdx], strip, c, initialPhase);
                    }
                    break;

                case neutral:
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
                        animations.redFlash.init(now, states[i][stateIdx], strip, RGBW_SUPPORT);
                        animationQueue.add(&animations.solid);
                        stateIdx = animationQueue.lastIdx();
                        animations.solid.init(now, states[i][stateIdx], strip, c);
                    }
                    break;

                default:
                    Serial.print("Invalid owner "); Serial.println(owner);
                    break;
            }
//        Serial.print((char *)command); Serial.print(" - ");Serial.print(command[0],DEC);Serial.print(": "); 
//        Serial.print("owner "); Serial.print(owner,DEC); Serial.print(", percent "); Serial.println(percent,DEC); 
        }
    }

  if(now > last_millis + STRIP_TIME_COUNT )
  {
    last_millis = now; // every X milliseconds we will check this direction
    
    strip.setPin(dir+BASE_PIN);  // pick the string
//    strip.setPin(BASE_PIN);  // pick the string

        QueueType& animationQueue = animationQueues[dir];
        unsigned int queueSize = animationQueue.size();
        if (queueSize > 1) {
            if (animationQueue.peek()->done(now, states[dir][animationQueue.currIdx()])) {
                animationQueue.remove();
                queueSize--;
                animationQueue.peek()->start(now, states[dir][animationQueue.currIdx()]);
            }
        }
        if (queueSize > 0) {
            //strip.setBrightness(255);
            strip.setBrightness((uint8_t)((uint16_t)(255*(percent/100.0))));
            //pCurrAnimation->doFrame(states[0], strip);
            animationQueue.peek()->doFrame(now, states[dir][animationQueue.currIdx()], strip);
        }


    // Update one strand each time through the loop
    dir = (dir + 1) % NUM_STRINGS;
  }


  // smoke on/off
  if( now != smoke_millis )
  {
    smoke_millis = now;

    if( SmokeActive(++smoke1_timer, percent) )
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
    if( SmokeActive(++smoke2_timer, percent) )
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



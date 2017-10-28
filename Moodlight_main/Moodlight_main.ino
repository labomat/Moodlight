/*

    M O O D L I G H T

    Steuerung einer 11x10 Pixel großen Matrix von WS2812 RGB-LEDS
    mit diversen Animationen auf einem Teensy LC

    11x11 pixel led 2812 matrix with animations
    powered by Teensy LC and ESP8266

    CC-BY SA 2016 Kai Laborenz


*/

// animation control

#include <Encoder.h>        // Rotary encoder
#include <Bounce.h>         // bebounce routine for switches
#include <FastLED.h>        // FastLED Animation library
#include <Wire.h>

#ifdef __AVR__
#include <avr/power.h>
#endif

Encoder myEnc(5, 6);        // initialize Encoder
long oldEncoderPos  = 0;

#define SWITCH_PIN 7        // rotary encoder switch pin

Bounce modeSwitch = Bounce( SWITCH_PIN, 1000 ); // Instantiate Bounce object with 500 millisecond debounce time
int mode = 1;               // animation modes
const int maxModes = 5;     // number of modes (cycling, starting from 1 so 4 modes)

int animDelay = 0;               // delay of animation loop in ms (controlled by encoder)
const int animDelaySplash = 50;
const int animDelayParty = 250;  // delay start value for animation "Party Time"
const int animDelaytwinkle = 20; // delay start value for animation "Twinkle"
const int animDelayRainbow = 50; // delay start value for animation "Rainbow"

const int minanimDelay = 10;      // minimum delay
const int maxanimDelay = 1000;    // maximum delay

bool wake = 0;

#define LED_PIN 17          // controlpin for ws2812
#define NUM_LEDS 121        // number of leds in matrix
#define NUM_ROWS 11         // number of rows in matrix
#define NUM_COLS 11         // number of cols in matrix

#define COLOR_ORDER GRB
#define CHIPSET     WS2812
#define FRAMES_PER_SECOND 60

CRGB leds[NUM_LEDS];

uint8_t gHue = 0;               // rotating "base color" for fire animation

bool gReverseDirection = true;  // for twinkling animation

//  Twinkling 'holiday' lights that fade up and down in brightness.
//  Colors are chosen from a palette; a few palettes are provided.
//
//  The basic operation is that all pixels stay black until they
//  are 'seeded' with a relatively dim color.  The dim colors
//  are repeatedly brightened until they reach full brightness, then
//  are darkened repeatedly until they are fully black again.
//
//  A set of 'directionFlags' is used to track whether a given
//  pixel is presently brightening up or darkening down.
//
//  Darkening colors accurately is relatively easy: scale down the
//  existing color channel values.  Brightening colors is a bit more
//  error prone, as there's some loss of precision.  If your colors
//  aren't coming our 'right' at full brightness, try increasing the
//  STARTING_BRIGHTNESS value.
//
//  -Mark Kriegsman, December 2014

#define MASTER_BRIGHTNESS   96
#define STARTING_BRIGHTNESS 64
#define FADE_IN_SPEED       32
#define FADE_OUT_SPEED      20
#define DENSITY            255


int bufferCount;    // Anzahl der eingelesenen Zeichen
char buffer[80];    // Serial Input-Buffer
byte led = 13;

int blinktime = 2000;
int waittime = 1000;
boolean ledstate = false;
long switchtime = 0;

String serialCommand = "";         // a string to hold incoming data
boolean commandComplete = false;  // whether the string is complete


void setup() {

  //pinMode(ESP8266_CHPD, OUTPUT);
  //digitalWrite(ESP8266_CHPD,HIGH);

  Serial.begin(19200);
  Wire.begin(9);
  Wire.onReceive(receiveEvent); 
  //Serial2.begin(115200);             // ESP8266

  pinMode(SWITCH_PIN, INPUT);
  attachInterrupt(SWITCH_PIN, changeMode, RISING);

  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection(TypicalLEDStrip);

  delay(1000);
  Serial.println("Let's Go!");

  delay(1000);

}

void loop() {

  switch (mode) {


    ///////    ANIMATION 1    ///////

    case 1:

      Serial.println("Flat Color");
      FastLED.setBrightness(70);

      while (mode == 1) {

        uint16_t i, hue;

        for (i = 0; i < NUM_LEDS; i++) {
          //leds[i] = CHSV(hue, 255, 255); // set random color
          fill_solid(leds, NUM_LEDS, CHSV(hue, 255, 255));
        }

        // read hue from encoder
        long newEncoderPos = constrain((myEnc.read()), 0, 255);
        
        if (newEncoderPos != oldEncoderPos) {
          oldEncoderPos = newEncoderPos;
        }
        hue = newEncoderPos;
        Serial.println(hue);

        FastLED.show();
        FastLED.delay(1000 / FRAMES_PER_SECOND);

      }
      break;


    ///////    ANIMATION 1    ///////

    case 2:

      Serial.println("  !");
      FastLED.setBrightness(123);
      animDelay = animDelaySplash;
      myEnc.write(animDelay);

      while (mode == 2) {

        // eight colored dots, weaving in and out of sync with each other
        fadeToBlackBy( leds, NUM_LEDS, 20);

        byte dothue = 0;
        for ( int i = 0; i < 8; i++) {
          leds[beatsin16(i + 7, 0, NUM_LEDS)] |= CHSV(dothue, 200, 255);
          dothue += 32;

          FastLED.show();
          delay(animDelay);

          long newEncoderPos = constrain((myEnc.read()), minanimDelay, maxanimDelay);

          if (newEncoderPos != oldEncoderPos) {
            oldEncoderPos = newEncoderPos;
          }
          Serial.println(newEncoderPos);
          animDelay = newEncoderPos;
        }
      }
      break;


    ///////    ANIMATION 2    ///////

    case 3:

      Serial.println("Party Time!");
      FastLED.setBrightness(123);

      uint16_t i;
      animDelay = animDelayParty;
      myEnc.write(animDelay);
      Serial.println(animDelay);
      
      while (mode == 3) {
        for (i = 0; i < NUM_LEDS; i++) {
          leds[i] = CHSV(random8(), 255, 255); // set random color
        }

        FastLED.show();
        Serial.println(animDelay);
        delay(animDelay);
        

        // read animation speed delay from encoder
        long newEncoderPos = constrain((2*myEnc.read()), minanimDelay, maxanimDelay);
        
        if (newEncoderPos != oldEncoderPos) {
          Serial.print("Encoder read: ");
          Serial.println(myEnc.read());
          oldEncoderPos = newEncoderPos;
        }
        animDelay = newEncoderPos;
        Serial.print("Anim delay: ");
        Serial.println(animDelay);
      } // end while
      FastLED.clear();
      break;


    ///////    ANIMATION 3    ///////

    case 4:

      Serial.println("Twinkling Lights!");

      uint16_t j;
      animDelay = animDelaytwinkle;
      myEnc.write(animDelay);
      Serial.println(animDelay);

      while (mode == 4) {

        chooseColorPalette();
        colortwinkles();
        FastLED.show();
        FastLED.delay(animDelay);

        long newEncoderPos = constrain((myEnc.read()/3), minanimDelay, maxanimDelay);
        if (newEncoderPos != oldEncoderPos) {
          oldEncoderPos = newEncoderPos;
        }
        Serial.println(newEncoderPos);
        animDelay = newEncoderPos;
        j = j + 1;

      }
      break;

    ///////    ANIMATION 4    ///////

    case 5:

      Serial.println("Rainbow!");
      FastLED.setBrightness(123);
      
      //uint16_t j;
      animDelay = animDelayRainbow;
      myEnc.write(animDelay);
      Serial.println(animDelay);

      while (mode == 5) {
        uint32_t ms = millis();
        int32_t yHueDelta32 = ((int32_t)cos16( ms * 27 ) * (350 / NUM_COLS));
        int32_t xHueDelta32 = ((int32_t)cos16( ms * 39 ) * (310 / NUM_ROWS));
        DrawOneFrame( ms / 65536, yHueDelta32 / 32768, xHueDelta32 / 32768);
        FastLED.show();

        long newEncoderPos = constrain((myEnc.read()/3), minanimDelay, maxanimDelay);

        if (newEncoderPos != oldEncoderPos) {
          oldEncoderPos = newEncoderPos;
        }
        Serial.println(newEncoderPos);
        ms = ms * newEncoderPos;
      }

  }   // end switch
}     // end main loop

void DrawOneFrame( byte startHue8, int8_t yHueDelta8, int8_t xHueDelta8)
{
  byte lineStartHue = startHue8;
  for ( byte y = 0; y < NUM_ROWS; y++) {
    lineStartHue += yHueDelta8;
    byte pixelHue = lineStartHue;
    for ( byte x = 0; x < NUM_COLS; x++) {
      pixelHue += xHueDelta8;
      leds[ XY(x, y)]  = CHSV( pixelHue, 255, 255);
    }
  }
}
// Helper function that translates from x, y into an index into the LED array
// Handles both 'row order' and 'serpentine' pixel layouts.
uint16_t XY( uint8_t x, uint8_t y)
{
  uint16_t i;

  if ( gReverseDirection == false) {
    i = (y * NUM_COLS) + x;
  } else {
    if ( y & 0x01) {
      // Odd rows run backwards
      uint8_t reverseX = (NUM_COLS - 1) - x;
      i = (y * NUM_COLS) + reverseX;
    } else {
      // Even rows run forwards
      i = (y * NUM_COLS) + x;
    }
  }

  return i;
}



/////  ANIMATION MODES  /////

// Fire2012 by Mark Kriegsman, July 2012
// as part of "Five Elements" shown here: http://youtu.be/knWiGsmgycY
////
// This basic one-dimensional 'fire' simulation works roughly as follows:
// There's a underlying array of 'heat' cells, that model the temperature
// at each point along the line.  Every cycle through the simulation,
// four steps are performed:
//  1) All cells cool down a little bit, losing heat to the air
//  2) The heat from each cell drifts 'up' and diffuses a little
//  3) Sometimes randomly new 'sparks' of heat are added at the bottom
//  4) The heat from each cell is rendered as a color into the leds array
//     The heat-to-color mapping uses a black-body radiation approximation.
//
// Temperature is in arbitrary units from 0 (cold black) to 255 (white hot).
//
// There are two main parameters you can play with to control the look and
// feel of your fire: COOLING (used in step 1 above), and SPARKING (used
// in step 3 above).
//
// COOLING: How much does the air cool as it rises?
// Less cooling = taller flames.  More cooling = shorter flames.
// Default 50, suggested range 20-100
#define COOLING  55

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// Default 120, suggested range 50-200.
#define SPARKING 120

void Fire2012()
{
  // Array of temperature readings at each simulation cell
  static byte heat[NUM_LEDS];

  // Step 1.  Cool down every cell a little
  for ( int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8( heat[i],  random8(0, ((COOLING * 10) / NUM_LEDS) + 2));
  }

  // Step 2.  Heat from each cell drifts 'up' and diffuses a little
  for ( int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2] ) / 3;
  }

  // Step 3.  Randomly ignite new 'sparks' of heat near the bottom
  if ( random8() < SPARKING ) {
    int y = random8(7);
    heat[y] = qadd8( heat[y], random8(160, 255) );
  }

  // Step 4.  Map from heat cells to LED colors
  for ( int j = 0; j < NUM_LEDS; j++) {
    CRGB color = HeatColor( heat[j]);
    int pixelnumber;
    if ( gReverseDirection ) {
      pixelnumber = (NUM_LEDS - 1) - j;
    } else {
      pixelnumber = j;
    }
    leds[pixelnumber] = color;
  }
  FastLED.show(); // display this frame
  FastLED.delay(1000 / FRAMES_PER_SECOND);
}


void rainbow()
{
  // FastLED's built-in rainbow generator
  fill_rainbow( leds, NUM_LEDS, gHue, 5);
  FastLED.show(); // display this frame
}


/// Routines for animation tinkling lights ///

CRGBPalette16 gPalette;

void chooseColorPalette()
{
  uint8_t numberOfPalettes = 5;
  uint8_t secondsPerPalette = 10;
  uint8_t whichPalette = (millis() / (1000 * secondsPerPalette)) % numberOfPalettes;

  CRGB r(CRGB::Red), b(CRGB::Blue), w(85, 85, 85), g(CRGB::Green), W(CRGB::White), l(0xE1A024);

  switch ( whichPalette) {
    case 0: // Red, Green, and White
      gPalette = CRGBPalette16( r, r, r, r, r, r, r, r, g, g, g, g, w, w, w, w );
      break;
    case 1: // Blue and White
      //gPalette = CRGBPalette16( b,b,b,b, b,b,b,b, w,w,w,w, w,w,w,w );
      gPalette = CloudColors_p; // Blues and whites!
      break;
    case 2: // Rainbow of colors
      gPalette = RainbowColors_p;
      break;
    case 3: // Incandescent "fairy lights"
      gPalette = CRGBPalette16( l, l, l, l, l, l, l, l, l, l, l, l, l, l, l, l );
      break;
    case 4: // Snow
      gPalette = CRGBPalette16( W, W, W, W, w, w, w, w, w, w, w, w, w, w, w, w );
      break;
  }
}

enum { GETTING_DARKER = 0, GETTING_BRIGHTER = 1 };

void colortwinkles()
{
  // Make each pixel brighter or darker, depending on
  // its 'direction' flag.
  brightenOrDarkenEachPixel( FADE_IN_SPEED, FADE_OUT_SPEED);

  // Now consider adding a new random twinkle
  if ( random8() < DENSITY ) {
    int pos = random16(NUM_LEDS);
    if ( !leds[pos]) {
      leds[pos] = ColorFromPalette( gPalette, random8(), STARTING_BRIGHTNESS, NOBLEND);
      setPixelDirection(pos, GETTING_BRIGHTER);
    }
  }
}

void brightenOrDarkenEachPixel( fract8 fadeUpAmount, fract8 fadeDownAmount)
{
  for ( uint16_t i = 0; i < NUM_LEDS; i++) {
    if ( getPixelDirection(i) == GETTING_DARKER) {
      // This pixel is getting darker
      leds[i] = makeDarker( leds[i], fadeDownAmount);
    } else {
      // This pixel is getting brighter
      leds[i] = makeBrighter( leds[i], fadeUpAmount);
      // now check to see if we've maxxed out the brightness
      if ( leds[i].r == 255 || leds[i].g == 255 || leds[i].b == 255) {
        // if so, turn around and start getting darker
        setPixelDirection(i, GETTING_DARKER);
      }
    }
  }
}

CRGB makeBrighter( const CRGB& color, fract8 howMuchBrighter)
{
  CRGB incrementalColor = color;
  incrementalColor.nscale8( howMuchBrighter);
  return color + incrementalColor;
}

CRGB makeDarker( const CRGB& color, fract8 howMuchDarker)
{
  CRGB newcolor = color;
  newcolor.nscale8( 255 - howMuchDarker);
  return newcolor;
}

uint8_t  directionFlags[ (NUM_LEDS + 7) / 8];

bool getPixelDirection( uint16_t i) {
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  return bitRead( directionFlags[index], bitNum);
}
void setPixelDirection( uint16_t i, bool dir) {
  uint16_t index = i / 8;
  uint8_t  bitNum = i & 0x07;
  bitWrite( directionFlags[index], bitNum, dir);
}



/////  I2C + SWITCH SUBROUTINES  /////

void changeMode() {
  // changes mode after encoder button is pressed

  modeSwitch.update();
  int value = modeSwitch.read();
  //Serial.println(value);
  if ( value == HIGH ) {
    mode = mode + 1;
    if (mode > maxModes) mode = 1;
    if (mode < 1) mode = maxModes;
  }
  delay(1000);
  Serial.print("Switched to Modus: ");
  Serial.println(mode);
}


void receiveEvent(int bytes) {
 mode = 0;
 mode = Wire.read();
}



/*
 * Keudn's custom LED matrix controller, coded for Billy "Veryhandsomebilly" Mills.
 * Designed to control n number of LED strips with n LEDs on each strip, each strip
 * connected to a separate data pin on the arduino. Uses FFT to do frequency analysis.
 * Built for Teensy 3.1/3.2
 */

#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <FastLED.h>
#include <Gaussian.h>

AudioInputAnalog adc0(A0);          //audio input pin
AudioAnalyzeFFT1024 fft;
AudioConnection patchCord1(adc0, fft);

const int NUM_STRIPS = 8;           //number of LED strips connected to arduino
float level[NUM_STRIPS];            //an array to hold the FFT value for each strip

#define NUM_LEDS 20                 //change depending on number of LEDs in each strip
#define DATA_PIN0 1
#define DATA_PIN1 2
#define DATA_PIN2 3
#define DATA_PIN3 4
#define DATA_PIN4 5
#define DATA_PIN5 6
#define DATA_PIN6 7
#define DATA_PIN7 8

const int buttonDelay = 500;        //time in ms for a button press to be a long hold
const int buttonPin = 0;
bool buttonState = HIGH;
bool buttonLast = HIGH;
bool buttonWasLow = true;
unsigned long pressTime = 0;

int mode = 0;
int H = 0;
int h = 0;                          //used for random h value in audio reactive mode
float S = 0;
const int V = 180;                  //max V value in HSV
int minSat = 128;                   //minimum saturation the random saturation can be
Gaussian sat = Gaussian(1, 0.40);   //creates gaussian with mean of 1 and variance of 0.4

CRGB leds0[NUM_LEDS];
CRGB leds1[NUM_LEDS];
CRGB leds2[NUM_LEDS];
CRGB leds3[NUM_LEDS];
CRGB leds4[NUM_LEDS];
CRGB leds5[NUM_LEDS];
CRGB leds6[NUM_LEDS];
CRGB leds7[NUM_LEDS];

int LED[NUM_STRIPS][NUM_LEDS];      //main LED array, each data position corresponds to and LED on your matrix, organized in x strips and y LEDs
const int fadeSteps = 32;           //number of cycles an LED can be lit for, higher values create a longer ghost effect, fades linearly each step. This does not work great for low values of V
const int fadeRate = 1;             //rate at which LEDs fade in audio reactivity mode
const int topSpectrumNum = 5;       //how many LEDs do you want to be a different color on the top of the audio spectrum mode?
bool audioSingleColor = false;      //do you want single color mode for audio?
const float audioGain = 1.5;          //increase audio gain by x multiple of V

//=======================================================

void setup() {
  FastLED.addLeds<NEOPIXEL, DATA_PIN0>(leds0, NUM_LEDS);    //initalize LED strips
  FastLED.addLeds<NEOPIXEL, DATA_PIN1>(leds1, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN2>(leds2, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN3>(leds3, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN4>(leds4, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN5>(leds5, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN6>(leds6, NUM_LEDS);
  FastLED.addLeds<NEOPIXEL, DATA_PIN7>(leds7, NUM_LEDS);

  Serial.begin(115200);

  pinMode(buttonPin, INPUT);

  HSVRand();   //generate first random H value

  // Audio requires memory to work.
  AudioMemory(12);
  Serial.print("In mode: ");
  Serial.print("mode");
  Serial.println();
  allOff();
}

//=======================================================

void loop() {
  if (digitalRead(buttonPin) == LOW && buttonWasLow)  {    //button is unpressed, continue normally, default state program in here
    buttonWasLow = true;
    program();
  }
  else if (digitalRead(buttonPin) == HIGH && buttonWasLow)  {   //button has just been pressed, record press time
    buttonWasLow = false;
    pressTime = millis();
    allOff();
  }
  else if (digitalRead(buttonPin) == LOW && !buttonWasLow)    //button was released just now, determine how long it was pressed
  {
    if (millis() - pressTime < buttonDelay && millis() - pressTime > 0)  //if it was a short press
    {
      Serial.print("Short press! Making new H and S values");
      Serial.println();
      HSVRand();
    }
    else if (millis() - pressTime > buttonDelay && millis() - pressTime > 0)  //long press
    {
      mode++;
      HSVRand();
      Serial.print("Long press! Going to next mode!");
      Serial.println();
    }
    buttonWasLow = true;
  }
}

//======================Main Program======================

void program()
{
  switch (mode)
  {
    case 0:     //default mode, all white lamp
      H = 0;
      S = 0;
      allHSV(0,0);
      break;
      
    case 1:     //two tone sound reactivity
      audioSingleColor = false;
      audioReact();
      break;

    case 2:     //single color sound reactivity
      audioSingleColor = true;
      audioReact();
      break;

    case 3:     //solid color sound reactivity, pulses to audio

      break;

    case 4:     //single random color
      allHSV(0, 0);
      break;

    case 5:     //color fading
      allHSV(0, 0);
      H++;
      S = 255;
      delay(100);
      break;

    case 6:     //not a mode, loops the switch back to start, must be last
      mode = 0;
      allOff();
      break;
  }
}

//======================Audio Functions======================

void audioReact()   //computes FFT values for each bin/strip
{
  //read in FFT values for each bin
  if (fft.available()) {
    level[0] = fft.read(0,3);
    level[1] = fft.read(4, 5);
    level[2] = fft.read(6, 18);
    level[3] = fft.read(19, 39);
    level[4] = fft.read(40, 70);
    level[5] = fft.read(71, 131);
    level[6] = fft.read(132, 257);
    level[7] = fft.read(258, 511);
  }

  /* Default 16 bin FFT
  level[0] = fft.read(0);
  level[1] = fft.read(1);
  level[2] = fft.read(2, 3);
  level[3] = fft.read(4, 6);
  level[4] = fft.read(7, 10);
  level[5] = fft.read(11, 15);
  level[6] = fft.read(16, 22);
  level[7] = fft.read(23, 32);
  level[8] = fft.read(33, 46);
  level[9] = fft.read(47, 66);
  level[10] = fft.read(67, 93);
  level[11] = fft.read(94, 131);
  level[12] = fft.read(132, 184);
  level[13] = fft.read(185, 257);
  level[14] = fft.read(258, 359);
  level[15] = fft.read(360, 511);
  */

  //make sure no bin goes over 1 and set noise floor
  for(int x = 0; x < NUM_STRIPS; x++)
    {
      if(level[x] > 1)
      {
        level[x] = 1;
      }

      if(level[x] < 0.10)
        {
          level[x] = 0;
        }
    }
    
  audioHSV();

  //Used to debug FFT data
  /*Serial.print("--FFT Data--");
  Serial.println();
  for(int i = 0; i < NUM_STRIPS; i++)
    {
      Serial.print(level[i]);
      Serial.print(" ");
    }
  Serial.println();
  Serial.println();*/
}

void audioHSV()   //takes FFT values and computes entire LED array frame by frame
{
  int totalLED[NUM_STRIPS];     //intialize array to store how many LEDs on each strip to be lit
  float ran = H + h;            //global hue + random hue increment to make top 5 LEDs a different color
  int v;
  if (ran > 255)
    {
      ran = ran - 255; //flips value over
    }
  
  for(int x = 0; x < NUM_STRIPS; x++)
    {
      totalLED[x] = level[x]*NUM_LEDS*audioGain - 1;            //I know I am truncating to integer LED values, but partially lighting the last LED doesn't look good anyway, better to simply fully light them
    }


  for(int x = 0; x < NUM_STRIPS; x++)                 //go through every value in main LED array
    {
      for(int y = 0; y < NUM_LEDS; y++)
        {
          if((LED[x][y] > 0) && (y > totalLED[x]))    //if the value is nonzero and greater than the current total max LEDs for that strip to be lit, subtract fade rate
            {
              LED[x][y] = LED[x][y] - fadeRate;
            }
          if(y <= totalLED[x])                        //if LED is suppose to be lit this cycle, set it to max brightness
            {
              LED[x][y] = fadeSteps;
            }

          v = static_cast<int>(V * (static_cast<float>(LED[x][y])/static_cast<float>(fadeSteps)));    //set brightness for the pixel to be the max brightness V * the fraction of the pixel value in the array over what its max value can be, creates a linear fade effect
          
          if(y >= (NUM_LEDS - topSpectrumNum) && audioSingleColor == false)    //when not in single color audio mode, set top LEDs to a different color
            {
              setSingleHSV(x, y, ran, v);
            }
          else if(y > 0)
            {
              setSingleHSV(x, y, H, v);
            }
        }
    }
  FastLED.show();

  //Used to debug the main LED array
  /*for(int y = NUM_LEDS - 1; y >= 0; y--)
    {
      for(int x = 0; x < NUM_STRIPS; x++)
        {
          Serial.print(LED[x][y]);
          Serial.print(" ");
        }
      Serial.println();
    }
  Serial.print("----------------------------------");
  Serial.println();*/
}

//======================Base LED Functions======================

void allRGB(int r, int g, int b, int pixdelay, int stripdelay)    //sets all LEDs on all strips to same RGB color
{
  for (int i = 0; i < NUM_STRIPS; i++)
  {
    stripRGB(i, r, g, b, pixdelay);
    delay(stripdelay);
  }
}

void allHSV(int pixdelay, int stripdelay)                         //sets all LEDs on all strips to same HSV color
{
  for (int i = 0; i < NUM_STRIPS; i++)
  {
    stripHSV(i, pixdelay);
    delay(stripdelay);
  }
}

void HSVRand()                                                    //generates a random H and S value
{
  H = random(0, 255);
  h = random(48,200); //used in audio reactive mode
  float randsat = sat.random();
  if (randsat <= 1)
  {
    S = 255. * randsat;
  }
  else if (randsat > 1)
  {
    S = 255. * (1. - (randsat - 1.));
  }
  if (S < minSat)   //clip saturation at a minimum
  {
    S = minSat;
  }

  Serial.print("H: ");
  Serial.print(H);
  Serial.print("  S: ");
  Serial.print(S);
  Serial.println();
}

void stripOff(int strip)                                           //turns off entire specified strip
{
  stripRGB(strip, 0, 0, 0, 0);
}

void allOff()                                                      //turns everything off
{
  for (int i = 0; i < NUM_STRIPS; i++)
  {
    stripOff(i);
  }
}

void stripHSV(int strip, int pixdelay)                             //set specified strip HSV
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (strip == 0)
    {
      leds0[i] = CHSV(H, S, V);
    }
    else if (strip == 1)
    {
      leds1[i] = CHSV(H, S, V);
    }
    else if (strip == 2)
    {
      leds2[i] = CHSV(H, S, V);
    }
    else if (strip == 3)
    {
      leds3[i] = CHSV(H, S, V);
    }
    else if (strip == 4)
    {
      leds4[i] = CHSV(H, S, V);
    }
    else if (strip == 5)
    {
      leds5[i] = CHSV(H, S, V);
    }
    else if (strip == 6)
    {
      leds6[i] = CHSV(H, S, V);
    }
    else if (strip == 7)
    {
      leds7[i] = CHSV(H, S, V);
    }

    if (pixdelay > 0)
    {
      FastLED.show();
      delay(pixdelay);
    }
  }
  if (pixdelay == 0)
  {
    FastLED.show();
  }
}

void stripRGB(int strip, int r, int g, int b, int pixdelay)           //set specified strip RGB
{
  for (int i = 0; i < NUM_LEDS; i++)
  {
    if (strip == 0)
    {
      leds0[i].setRGB(r, g, b);
    }
    else if (strip == 1)
    {
      leds1[i].setRGB(r, g, b);
    }
    else if (strip == 2)
    {
      leds2[i].setRGB(r, g, b);
    }
    else if (strip == 3)
    {
      leds3[i].setRGB(r, g, b);
    }
    else if (strip == 4)
    {
      leds4[i].setRGB(r, g, b);
    }
    else if (strip == 5)
    {
      leds5[i].setRGB(r, g, b);
    }
    else if (strip == 6)
    {
      leds6[i].setRGB(r, g, b);
    }
    else if (strip == 7)
    {
      leds7[i].setRGB(r, g, b);
    }

    if (pixdelay > 0)
    {
      FastLED.show();
      delay(pixdelay);
    }
  }
  if (pixdelay == 0)
  {
    FastLED.show();
  }
}

void setSingleHSV(int strip, int i, int h, int v)                     //set a single pixel with the non-global V and H! Does not use global V and H!
{
    if (strip == 0)
    {
      leds0[i] = CHSV(h, S, v);
    }
    else if (strip == 1)
    {
      leds1[i] = CHSV(h, S, v);
    }
    else if (strip == 2)
    {
      leds2[i] = CHSV(h, S, v);
    }
    else if (strip == 3)
    {
      leds3[i] = CHSV(h, S, v);
    }
    else if (strip == 4)
    {
      leds4[i] = CHSV(h, S, v);
    }
    else if (strip == 5)
    {
      leds5[i] = CHSV(h, S, v);
    }
    else if (strip == 6)
    {
      leds6[i] = CHSV(h, S, v);
    }
    else if (strip == 7)
    {
      leds7[i] = CHSV(h, S, v);
    }
}

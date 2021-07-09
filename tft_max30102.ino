/* MAX30102 Heart Rate Sensor with 1.8" TFT Display
 * by Roland Pelayo 
 * Complete Project Details https://www.teachmemicro.com
 * 
 * Required Libraries:
 * 
 *  - Adafruit GFX
 *  - Adafruit ST7735
 */

// include TFT and SPI libraries
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>
#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
MAX30105 particleSensor;

// pin definition for Arduino UNO
  #define TFT_CS        10
  #define TFT_RST        8 // Or set to -1 and connect to Arduino RESET pin
  #define TFT_DC         9

// global variables
const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred
float beatsPerMinute;
int beatAvg;
int heartBlinkRate;
char avgBpmOut[3];
String avgBpmOutAsString = "";
long lastUpdate;
bool goodToGo;

// create an instance of the library
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// heart bitmap
const unsigned char epd_bitmap_heart [] PROGMEM = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
  0x07, 0xf0, 0x0f, 0xe0, 0x07, 0xf0, 0x0f, 0xe0, 0x1c, 0xf8, 0x1f, 0xf8, 0x18, 0xfc, 0x3f, 0xf8, 
  0x39, 0xfe, 0x7f, 0xfc, 0x27, 0xff, 0xff, 0xfc, 0x27, 0xff, 0xff, 0xfc, 0x27, 0xff, 0xff, 0xfc, 
  0x27, 0xff, 0xff, 0xfc, 0x27, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfc, 0x3f, 0xff, 0xff, 0xfc, 
  0x1f, 0xff, 0xff, 0xf8, 0x1f, 0xff, 0xff, 0xf8, 0x07, 0xff, 0xff, 0xe0, 0x03, 0xff, 0xff, 0xc0, 
  0x01, 0xff, 0xff, 0x80, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x3f, 0xfc, 0x00, 
  0x00, 0x1f, 0xf8, 0x00, 0x00, 0x0f, 0xf0, 0x00, 0x00, 0x07, 0xe0, 0x00, 0x00, 0x03, 0xe0, 0x00, 
  0x00, 0x01, 0x80, 0x00, 0x00, 0x01, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

void setup() {
  Serial.begin(115200);
  //initialize the library
  tft.initR(INITR_BLACKTAB); 

  // clear the screen with a black background
  tft.fillScreen(ST77XX_BLACK);
  //set the text size
  tft.setTextSize(2);
  //set text color
  tft.setTextColor(ST77XX_WHITE);
  
  // Initialize sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    tft.setTextWrap(true);
    tft.print("MAX30105 was not found. Please check wiring/power. ");
    while (1);
  }
  
  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  
  // disable heart bitmap at start
  goodToGo = false;
}

void loop() {

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();
    goodToGo = true;

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }

    // display heart rate 
    Serial.println(beatAvg);
    avgBpmOutAsString = String(beatAvg);
    avgBpmOutAsString.toCharArray(avgBpmOut,6); 
    tft.setTextColor(ST77XX_WHITE,ST77XX_BLACK);
    tft.setCursor(10,30);
    tft.print("BPM = ");
    tft.setCursor(80,30);
    tft.print(avgBpmOut);
    tft.print(" ");
  }

  if(goodToGo){
    // blinking heart bitmap
    tft.drawBitmap(48,60, epd_bitmap_heart, 32, 32, ST77XX_WHITE);
    heartBlinkRate = beatsPerMinute / 60.0; //beats per second 
    if((millis() - lastUpdate) > (1000/heartBlinkRate)){
      tft.drawBitmap(48,60, epd_bitmap_heart, 32, 32, ST77XX_RED);
      lastUpdate = millis();
    } 
  }
}

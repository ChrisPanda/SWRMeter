/*
    SWRMeter(OLEDMeter) was written to utilize any 128x64 display. I have only seen marginal attempts to
    animate meters and I hope this one will set a standard. Please feel free to modify and share
    this code for any 128x64 LCD or OLED. OLEDMeter sketch was written for use with I2C SH1106.
    This code must be modified to work with other display devices.

    Working portion of code was taken from Adafruit Example Sound Level Sketch for the
    Adafruit Microphone Amplifier
    https://learn.adafruit.com/adafruit-microphone-amplifier-breakout/measuring-sound-levels

    Remaining code was written by Greg Stievenart with no claim to or any images or information
    provided in this code. Freely published May 26, 2016.

    Software to convert background mask to 128x64 at: http://www.ablab.in/image2glcd-software/

    IMPORTANT: Sound source must be grounded to the Arduino or other MCU's to work. Usually the
    base sleeve contact on TRS or TRRS connector is the ground.
*/
#include "fix_fft.h"
#include <Wire.h>                           // requried to run I2C SH1106
#include <Adafruit_GFX.h>                   // https://github.com/adafruit/Adafruit-GFX-Library
//#include <Adafruit_SSD1306.h>             // https://github.com/adafruit/Adafruit_SSD1306
#include <Adafruit_SH1106.h>                // https://github.com/wonho-maker/Adafruit_SH1106
#include "SWR.h"                            // New SWR Sensor
#include "SWRMeter.h"                       // New SWR Sensor
#include "image.h"                          // background mask image for OLED Meter
#include "utils.h"

#define VERSION_DATE  "20181207"

#define SCREEN_WIDTH 128                // OLED display width, in pixels
#define SCREEN_HEIGHT 64                // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4                    // reset required for SH1106

Adafruit_SH1106 display(OLED_RESET);    // reset required for SH1106
//Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define s_meter_input A0                // S-Meter analog input
#define vu_meter_input A1               // vu-Meter analog input
#define fwd_pwr_input A2                // forward power meter analog input
#define rev_pwr_input A3                // reverse power meter analog input

#define RecXmt_select 10                // receive transmit  select
#define Xmt_Mode_select 11              // transmit disp select  power or swr
#define Rec_Mode_select 12              // receive disp select  s-meter or vu meter
#define Alt_Mode_select 13              // alternate display mode select

#define  _S_MTR      0
#define  _VU_MTR     1
#define  _POWER      2
#define  _SWR        3
#define  _ALT        4

int lc = 0;

int analogInput = vu_meter_input;       // analog input being read at this time
int hMeter = 65;                        // horizontal center for needle animation
int vMeter = 85;                        // vertical center for needle animation (outside of dislay limits)
int rMeter = 80;                        // length of needle animation or arch of needle travel

char im[128], data[128];                //variables for the FFT
char x = 0, ylim = 60;                   //variables for drawing the graphics
int i = 0, val;
// sample window width in mS (50 mS = 20Hz)
unsigned int sample;                  // adc reading
unsigned int last_sample;
int dsp_Mode = _VU_MTR;

SWR swrData(fwd_pwr_input, rev_pwr_input);              // New SWR calculator

void setup() {
  Serial.begin(9600);

 i2cMeterSetup();
    
  // analog input for outside  source
  pinMode(s_meter_input,  INPUT);
  pinMode(vu_meter_input, INPUT);
  pinMode(fwd_pwr_input,  INPUT);
  pinMode(rev_pwr_input,  INPUT);

  pinMode(RecXmt_select,   INPUT_PULLUP);
  pinMode(Xmt_Mode_select, INPUT_PULLUP);
  pinMode(Rec_Mode_select, INPUT_PULLUP);
  pinMode(Alt_Mode_select, INPUT_PULLUP);

  // configure the new SWR algorithm
  swrData.MinPower(MIN_POWER);
  swrData.ScaleForward(FORWARD_SCALE);
  swrData.ScaleReflected(REFLECT_SCALE);

  display.begin(SH1106_SWITCHCAPVCC, 0x3C);             // needed for SH1106 display
  // display.begin(SSD1306_SWITCHCAPVCC, 0x3D);         // Address 0x3D for 128x64

  
  display.clearDisplay();                           // clears display from any library info displayed
  // display.invertDisplay(1);                      // option to invert display to black on white

  display.setTextSize(2);
  display.setTextColor(WHITE);
  strcpy_P(ioBuffer, PSTR("SWR Meter"));
  const uint8_t x = (SCREEN_WIDTH - strlen(ioBuffer)) / 2;
  Serial.print("x=");
  Serial.println(x);
  
  display.setCursor(0, 0);
  display.print(ioBuffer);
  //display.setCursor(x, 1);
  display.print(strcpy_P(ioBuffer, PSTR(VERSION_DATE)));
  display.display();

  dsp_Mode = _SWR;                               // start with default vu meter
  
  delay(2000);
  display.clearDisplay();
}

void loop() {
  swrData.Poll();                                   // spend all our spare time reading the transducer for new SWR

  /***********************************************************************
    Code to use digital pins to select display type
  ************************************************************************/
  //select_dspMode();

  while (dsp_Mode == _VU_MTR) {
    dsp_VU_Meter();
    //select_dspMode();
  }
  while (dsp_Mode == _S_MTR) {
    dsp_S_Meter();
    //select_dspMode();
  }
  while (dsp_Mode == _POWER) {
    dsp_PWR_Meter();
    //select_dspMode();
  }

  while (dsp_Mode == _SWR) {
    dsp_SWR_Meter();
    //select_dspMode();
  }

  while (dsp_Mode == _ALT) {
    dsp_FFT();
    //select_dspMode();
  }
}

/***********************************************************************
  Sample code to display each available display mode in sequence
***********************************************************************
  while (dsp_Mode == _VU_MTR) {
      dsp_VU_Meter();
      lc++;
      if (lc > 100) {
        dsp_Mode ++;
        lc = 0;
      }
  }
  while (dsp_Mode == _S_MTR) {
      dsp_S_Meter();
      lc++;
      if (lc > 100) {
        dsp_Mode++;
        lc = 0;
      }
  }
  while (dsp_Mode == _POWER) {
      dsp_PWR_Meter();
      lc++;
      if (lc > 100) {
        dsp_Mode++;
        lc = 0;
      }
  }
  while (dsp_Mode == _SWR  ) {
      dsp_SWR_Meter();
      lc++;
      if (lc > 100) {
        dsp_Mode++;
        lc = 0;
      }
  }
  while (dsp_Mode == _ALT ) {
      dsp_FFT();
      lc++;
      if (lc > 100) {
        dsp_Mode++;
        lc = 0;
      }
  }
  if (dsp_Mode > 4) {
      dsp_Mode = 0;
      lc = 0;
  }
************* END sample display mode *************/

// ***************** End of Loop ************

void  dsp_VU_Meter() {
  unsigned long startMillis = millis();                 // start of sample window
  int sampleWindow = 50;                                // 50 ms20 Hz.
  unsigned int PeaktoPeak = 0;                          // peak-to-peak level
  unsigned int SignalMax = 0;
  unsigned int SignalMin = 1024;
  while ( millis() - startMillis < sampleWindow ) {
    //sample = analogRead(vu_meter_input);
    sample = random (1024);      // test
    if (sample < 1024) {
      if (sample > SignalMax) {
        SignalMax = sample;                                // saves just the max levels
      }
      else if (sample < SignalMin) {
        SignalMin = sample;                                // saves just the min levels
      }
    }
  }
  PeaktoPeak = SignalMax - SignalMin;                      // max - min = peak-peak amplitude
  float MeterValue = PeaktoPeak * 330 / 1024;              // convert volts to arrow information

  /****************************************************
    End of code taken from Adafruit Sound Level Sketch
  *****************************************************/
  MeterValue = MeterValue - 34;                            // shifts needle to zero position
  display.clearDisplay();                                  // refresh display for next step
  display.drawBitmap(0, 0, VUMeter, 128, 64, WHITE);       // draws background
  int a1 = (hMeter + (sin(MeterValue / 47.296) * rMeter)); // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 47.296) * rMeter)); // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);         // draws needle
  display.display();
}

void dsp_PWR_Meter() {
  unsigned long startMillis = millis();  // start of sample window
  int sampleWindow = 50;                  // 50 ms 20 Hz.
  int adc = 0;
  int fwd_pwr = 0;
  int adc_offset = 0;                     // used to adjust meter zero if a dc offset to signal
  float MeterValue;  //  convert volts to arrow information
  while ( millis() - startMillis < sampleWindow ) {
    // get max value over 50 ms window
    adc = analogRead(fwd_pwr_input);
    if (adc < 1024) {
      if (adc > fwd_pwr) {
        fwd_pwr = adc;                                // saves just the max levels
      }
    }
  }
  MeterValue = map( fwd_pwr, 0, 1023 , 0, 62 - adc_offset);       // scale ADC value to meter width
  MeterValue = MeterValue - 36 + adc_offset;                          // shifts needle to zero position
  display.clearDisplay();                                  // refresh display for next step
  display.drawBitmap(0, 0, PowerMeter, 128, 64, WHITE);       // draws background
  int a1 = (hMeter + (sin(MeterValue / 47.296) * rMeter)); // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 47.296) * rMeter)); // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);         // draws needle
  display.display();
}

void dsp_S_Meter() {
  unsigned long startMillis = millis();           // start of sample window
  int sampleWindow = 50;                          // 50 ms 20 Hz.
  int adc = 0;
  int s_units = 0;
  int adc_offset = 0;                                               // used to adjust meter zero if a dc offset to signal
  float MeterValue;  //  convert volts to arrow information
  while ( millis() - startMillis < sampleWindow ) {
    // get max value over 50 ms window
    adc = analogRead(s_meter_input);
    if (adc < 1024) {
      if (adc > s_units) {
        s_units = adc;                                              // saves just the max levels
      }
    }
  }
  MeterValue = map( s_units, 0, 1023 , 0, 62 - adc_offset);         // scale ADC value  range to meter width
  MeterValue = MeterValue - 36 + adc_offset;                        // shifts needle to zero position
  display.clearDisplay();                                           // refresh display for next step
  display.drawBitmap(0, 0, S_Meter, 128, 64, WHITE);                // draws background
  int a1 = (hMeter + (sin(MeterValue / 47.296) * rMeter));          // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 47.296) * rMeter));          // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);                  // draws needle
  display.display();
}

void dsp_SWR_Meter() {
  unsigned long startMillis = millis();                             // start of sample window
  int sampleWindow = 50;                                            // 50 ms 20 Hz.
  int adcf = 0;
  int adcr = 0;
  float pwr_rev = 0;
  float pwr_fwd = 0;
  int adc_offset = 0;
  float swr = 0;                                                  // used to adjust meter zero if a dc offset to signal
  float MeterValue;                                               //  convert volts to arrow information
  /*
  while ( millis() - startMillis < sampleWindow ) {
    // get max value over 50 ms window
    //adcf = analogRead(fwd_pwr_input);
    //adcr = analogRead(rev_pwr_input);
    adcf = random (1024);
    adcr = random (1024);
    Serial.println(adcr);
    if (adcf < 1024) {
      if (adcf > pwr_fwd) {
        pwr_fwd = adcf;
        pwr_rev = adcr;    // saves just the max levels
      }
    }
  }
  */

//  swr = ((pwr_fwd + pwr_rev) / (pwr_fwd - pwr_rev) ) * 10;       
  swr = random (1023);    //test
  
  MeterValue = map(int(swr), 0, 1023 , 0, 62 - adc_offset);           // type cast to int and scale for meter range
  MeterValue = MeterValue - 36 + adc_offset;                          // shifts needle to zero position
  display.clearDisplay();                                             // refresh display for next step
  display.drawBitmap(0, 0, vswrMeter, 128, 64, WHITE);                // draws background
  int a1 = (hMeter + (sin(MeterValue / 47.296) * rMeter));            // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 47.296) * rMeter));            // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);                    // draws needle
  //display.display();

  //  SWR: read A/D and compute SWR
  FormatFloat(ioBuffer, sizeof(ioBuffer), swrData.Value());
  //Serial.print("SWR=");
  //Serial.println(ioBuffer);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 55);
  display.print("SWR=");
  display.print(ioBuffer);
  display.display();
  
  //  RAW: read A/D and output raw values
  snprintf(ioBuffer, sizeof(ioBuffer), "%d", swrData.ForwardRaw());
  // Serial.print("ForwardRaw=");
  // Serial.println(ioBuffer);
    
  snprintf(ioBuffer, sizeof(ioBuffer), "%d", swrData.ReflectedRaw());
  // Serial.print("ReflectedRaw=");
  // Serial.println(ioBuffer);
}

void select_dspMode() {
  if (digitalRead(RecXmt_select) == LOW) {
    if (digitalRead(Xmt_Mode_select) == LOW)dsp_Mode = _SWR;
    else dsp_Mode = _POWER;
  }
  else if (digitalRead(Rec_Mode_select) == LOW)
    dsp_Mode = _VU_MTR;
  else
    dsp_Mode = _S_MTR;
}

void dsp_FFT() {
  int min = 1024, max = 0;                            //set minumum & maximum ADC values
  for (i = 0; i < 128; i++) {                         //take 128 samples
    val = analogRead(vu_meter_input);                 //get audio from Analog 1
    val = random(1024); //test
    data[i] = val / 2 - 128;                          //each element of array is val/4-128
    im[i] = 0;                                        //
    if (val > max) max = val;                         //capture maximum level
    if (val < min) min = val;                         //capture minimum level
  };

  fix_fft(data, im, 7, 0);                            //perform the FFT on data

  display.clearDisplay();                             //clear display
  for (i = 1; i < 64; i++) {                          // In the current design, 60Hz and noise
    int dat = 3 * sqrt(data[i] * data[i] + im[i] * im[i]); //filter out noise and hum

    display.drawLine(i * 2 + x, ylim, i * 2 + x, ylim - dat, WHITE); // draw bar graphics for freqs above 500Hz to buffer
  };
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);                            //set cursor to top of screen
  display.print("  Audio Spectrum");                  //print title to buffer
  display.display();                                  //show the buffer
}

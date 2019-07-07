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
#include <Wire.h>
#include <Adafruit_GFX.h>                   // https://github.com/adafruit/Adafruit-GFX-Library
#include <Adafruit_SSD1306.h>               // https://github.com/adafruit/Adafruit_SSD1306
//#include <Adafruit_SH1106.h>              // https://github.com/wonho-maker/Adafruit_SH1106
#include <fix_fft.h>
#include <OneButton.h>
#include "define.h"
#include "SWR.h"                            // New SWR Sensor
#include "SWRMeter.h"                       // New SWR Sensor
#include "image.h"                          // background mask image for OLED Meter
#include "utils.h"
#include "menu.h"

#define SCREEN_WIDTH 128                // OLED display width, in pixels
#define SCREEN_HEIGHT 64                // OLED display height, in pixels

#ifdef SSD1306
// Declaration for SSD1306 display connected using software SPI (default case):
#define OLED_CLK    2
#define OLED_MOSI   4
#define OLED_RESET  6
#define OLED_DC     8
#define OLED_CS    10

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);
#else
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET 4                    // reset required for SH1106

Adafruit_SH1106 display(OLED_RESET);    // reset required for SH1106
#endif

#define s_meter_input  A6               // S-Meter analog input (from AGC line)
#define fwd_pwr_input  A8               // forward power meter analog input
#define rev_pwr_input  A9               // reverse power meter analog input
#define vu_meter_input A10              // vu-Meter analog input (from audio signal)

#define RecXmt_select   7               // receive transmit select (this can come from the PTT or key line)
#define Alt_Mode_select 8               // alternate display mode select

#define  _S_MTR      0
#define  _VU_MTR     1
#define  _POWER      2
#define  _SWR        3
#define  _ALT        4

OneButton button(A4, true);

int Xmt_Mode_select;                    // select swr or power when transmit display
int Rec_Mode_select;                    // select vu meter or s-meter when receive display
boolean menuEnable;
int analogInput = vu_meter_input;       // analog input being read at this time
int dsp_Mode = _VU_MTR;
const int hMeter = 65;                  // horizontal center for needle animation
const int vMeter = 85;                  // vertical center for needle animation (outside of dislay limits)
const int rMeter = 80;                  // length of needle animation or arch of needle travel

#ifdef SWR_H  
SWR swrData(fwd_pwr_input, rev_pwr_input);              // New SWR calculator
#endif

void setup() {
  Serial.begin(9600);

#ifdef I2C_H
  i2cMeterSetup();
#endif  
    
  // analog input for outside  source
  pinMode(s_meter_input,  INPUT);
  pinMode(vu_meter_input, INPUT);
  pinMode(fwd_pwr_input,  INPUT);
  pinMode(rev_pwr_input,  INPUT);
  pinMode(RecXmt_select,   INPUT_PULLUP);
  pinMode(Alt_Mode_select, INPUT_PULLUP);

#ifdef SWR_H
  // configure the new SWR algorithm
  swrData.MinPower(MIN_POWER);
  swrData.ScaleForward(FORWARD_SCALE);
  swrData.ScaleReflected(REFLECT_SCALE);
#endif

#ifdef SSD1306
  display.begin(SSD1306_SWITCHCAPVCC);         // Address 0x3D for 128x64
#else
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);             // needed for SH1106 display
#endif

  display.clearDisplay();                           // clears display from any library info displayed
  // display.invertDisplay(1);                      // option to invert display to black on white

  display.setTextSize(1);
  display.setTextColor(WHITE);
  strcpy_P(ioBuffer, PSTR("Analog Meter: "));
  const uint8_t x = (SCREEN_WIDTH - strlen(ioBuffer)) / 2;
  //Serial.print("x=");
  //Serial.println(x);
  
  display.setCursor(0, 20);
  display.print(ioBuffer);
  //display.setCursor(x, 1);
  display.print(strcpy_P(ioBuffer, PSTR(VERSION)));
  display.display();

  Xmt_Mode_select=0;                            // select default mode when transmit display
  Rec_Mode_select=0;                            // select  default mode when receive display
  dsp_Mode = _SWR;                              // start with default vu meter

#ifdef _ENABLE_MENU
  menuEnable = false;
  button.attachLongPressStart(showHideMenu);
  button.attachClick(changeMenuItem);
  button.attachDoubleClick(selectMenuItem);

  ms.get_root_menu().add_menu(&mu1);
  mu1.add_item(&mu1_mi1);
  mu1.add_item(&mu1_mi2);
  mu1.add_item(&mu1_back);
  mu1.add_item(&mu1_mi3);
  
  ms.get_root_menu().add_menu(&mu2);
  mu2.add_item(&mu2_mi1);
  mu2.add_item(&mu2_mi2);
  mu2.add_item(&mu2_back);
  mu2.add_item(&mu2_mi3);

  ms.get_root_menu().add_item(&mm_mi1);

  // set 80 msec. debouncing time. Default is 50 msec.
  button.setDebounceTicks(80);
#endif // _ENABLE_MENU

  delay(1500);
  display.clearDisplay();
}

void loop() {
  checkI2CnMenu();

  while (dsp_Mode == _VU_MTR) {
#ifdef _ENABLE_MENU
    if (!menuEnable)
#endif
      dsp_VU_Meter();
    checkI2CnMenu();
  }
  while (dsp_Mode == _S_MTR) {
#ifdef _ENABLE_MENU
    if (!menuEnable)
#endif
      dsp_S_Meter();
    checkI2CnMenu();
  }
  while (dsp_Mode == _POWER) {
#ifdef _ENABLE_MENU
    if (!menuEnable)
#endif
      dsp_PWR_Meter();
    checkI2CnMenu();
  }

  while (dsp_Mode == _SWR) {
#ifdef _ENABLE_MENU
    if (!menuEnable)
#endif
      dsp_SWR_Meter();
    checkI2CnMenu();
  }

  while (dsp_Mode == _ALT) {
#ifdef _ENABLE_MENU
    if (!menuEnable)
#endif
      dsp_FFT();
    checkI2CnMenu();
  }
}

// check I2C communication and Menu
void checkI2CnMenu() {
  button.tick();

#ifdef _ENABLE_MENU
  if (menuEnable) {
    ms.display();
    return;
  }
#endif

  // TR/RX check
  if (digitalRead(RecXmt_select) == LOW) {  // receiving mode
    if (Rec_Mode_select == 0)
      dsp_Mode = _VU_MTR;
    else
      dsp_Mode = _S_MTR;
  }
  else {                                    // transmission mode
    if (Xmt_Mode_select == 0)
      dsp_Mode = _SWR;
    else
      dsp_Mode = _POWER;
  }

#ifdef SWR_H
  swrData.Poll();                                   // spend all our spare time reading the transducer for new SWR
#endif

#ifdef I2C_H
  i2cMeterLoop();
#endif
}

void  dsp_VU_Meter() {
  unsigned long startMillis = millis();                 // start of sample window
  int sampleWindow = 50;                                // 50 ms20 Hz.
  unsigned int PeaktoPeak = 0;                          // peak-to-peak level
  unsigned int SignalMax = 0;
  unsigned int SignalMin = 1024;
  unsigned int sample;                                  // adc reading

  while ( millis() - startMillis < sampleWindow ) {
    sample = analogRead(vu_meter_input);
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

  while ( millis() - startMillis < sampleWindow ) {
    // get max value over 50 ms window
    adcf = analogRead(fwd_pwr_input);
    adcr = analogRead(rev_pwr_input);
    //Serial.println(adcr);
    if (adcf < 1024) {
      if (adcf > pwr_fwd) {
        pwr_fwd = adcf;
        pwr_rev = adcr;    // saves just the max levels
      }
    }
  }
  swr = ((pwr_fwd + pwr_rev) / (pwr_fwd - pwr_rev) ) * 10;

  MeterValue = map(int(swr), 0, 1023 , 0, 62 - adc_offset);           // type cast to int and scale for meter range
  MeterValue = MeterValue - 36 + adc_offset;                          // shifts needle to zero position
  display.clearDisplay();                                             // refresh display for next step
  display.drawBitmap(0, 0, vswrMeter, 128, 64, WHITE);                // draws background
  int a1 = (hMeter + (sin(MeterValue / 47.296) * rMeter));            // meter needle horizontal coordinate
  int a2 = (vMeter - (cos(MeterValue / 47.296) * rMeter));            // meter needle vertical coordinate
  display.drawLine(a1, a2, hMeter, vMeter, WHITE);                    // draws needle
  //display.display();

  //  SWR: read A/D and compute SWR
#ifdef SWR_H  
  FormatFloat(ioBuffer, sizeof(ioBuffer), swrData.Value());
#endif  
  FormatFloat(ioBuffer, sizeof(ioBuffer), swr);
  //Serial.print("SWR=");
  //Serial.println(ioBuffer);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 55);
  display.print("SWR=");
  display.print(ioBuffer);
  display.display();
  
  //  RAW: read A/D and output raw values
#ifdef SWR_H
  snprintf(ioBuffer, sizeof(ioBuffer), "%d", swrData.ForwardRaw());
  // Serial.print("ForwardRaw=");
  // Serial.println(ioBuffer);
    
  snprintf(ioBuffer, sizeof(ioBuffer), "%d", swrData.ReflectedRaw());
  // Serial.print("ReflectedRaw=");
  // Serial.println(ioBuffer);
#endif
}

void dsp_FFT() {
  int val;
  int min = 1024, max = 0;                          //set minumum & maximum ADC values
  char x = 0, ylim = 60;                            //variables for drawing the graphics
  char im[128], data[128];                          //variables for the FFT
  
  for (int i = 0; i < 128; i++) {                     //take 128 samples
    val = analogRead(vu_meter_input);                 //get audio from Analog 1
    data[i] = val / 2 - 128;                          //each element of array is val/4-128
    im[i] = 0;                                        //
    if (val > max) max = val;                         //capture maximum level
    if (val < min) min = val;                         //capture minimum level
  };

  fix_fft(data, im, 7, 0);                            //perform the FFT on data

  display.clearDisplay();                             //clear display
  for (int i = 1; i < 64; i++) {                          // In the current design, 60Hz and noise
    int dat = 3 * sqrt(data[i] * data[i] + im[i] * im[i]); //filter out noise and hum

    display.drawLine(i * 2 + x, ylim, i * 2 + x, ylim - dat, WHITE); // draw bar graphics for freqs above 500Hz to buffer
  };
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);                            //set cursor to top of screen
  display.print("  Audio Spectrum");                  //print title to buffer
  display.display();                                  //show the buffer
}

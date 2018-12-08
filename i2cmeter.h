/*
Configuration file for Nextion LCD and Control MCU
The parameter can be set according to the CPU used. 

KD8CEC, Ian Lee
-----------------------------------------------------------------------

**********************************************************************/

#include <arduino.h>

//================================================================
//COMMUNICATION SECTION
//================================================================
#define USE_SW_SERIAL

extern void SWSerial_Write(uint8_t b);
extern void SWSerial_Print(uint8_t *b);

#ifdef USE_SW_SERIAL
  extern void SWSerial_Begin(long speedBaud);
  extern int SWSerial_Available(void);
  extern int SWSerial_Read(void);
#else

#define PRINT_MAX_LENGTH 30
#endif

//================================================================
//FFT and Decode Morse
//================================================================
#define FFTSIZE 64
#define SAMPLE_PREQUENCY 6000
#define SAMPLESIZE (FFTSIZE * 2)
#define DECODE_MORSE_SAMPLESIZE 48

extern uint8_t cwDecodeHz;
extern int magnitudelimit_low;

//================================================================
//EEPROM Section
//================================================================
#define MAX_FORWARD_BUFF_LENGTH 128
#define EEPROM_DSPTYPE      100
#define EEPROM_SMETER_UART  111
#define EEPROM_SMETER_TIME  112

#define EEPROM_CW_FREQ      120
//#define EEPROM_CW_MAG_LIMIT 121
#define EEPROM_CW_MAG_LOW   122
#define EEPROM_CW_NBTIME    126
#define EEPROM_RTTYDECODEHZ 130

//================================================================
//DEFINE for I2C Command
//================================================================
//S-Meter Address
#define I2CMETER_ADDR     0x58  //changed from 0x6A
//VALUE TYPE============================================
//Signal
#define I2CMETER_CALCS    0x59 //Calculated Signal Meter
#define I2CMETER_UNCALCS  0x58 //Uncalculated Signal Meter

//Power
#define I2CMETER_CALCP    0x57 //Calculated Power Meter
#define I2CMETER_UNCALCP  0x56 //UnCalculated Power Meter

//SWR
#define I2CMETER_CALCR    0x55 //Calculated SWR Meter
#define I2CMETER_UNCALCR  0x54 //Uncalculated SWR Meter

#define SIGNAL_METER_ADC  A7
#define POWER_METER_ADC   A3
#define SWR_METER_ADC     A2



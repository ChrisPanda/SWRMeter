/*
FFT, CW Decode for uBITX
KD8CEC, Ian Lee
-----------------------------------------------------------------------
License : See fftfunctions.cpp for FFT and CW Decode.
**********************************************************************/

#include <EEPROM.h>
#include "i2cmeter.h"

#define SWS_HEADER_CHAR_TYPE 'c'  //1Byte Protocol Prefix
#define SWS_HEADER_INT_TYPE  'v'  //Numeric Protocol Prefex
#define SWS_HEADER_STR_TYPE  's'  //for TEXT Line compatiable Character LCD Control

//Control must have prefix 'v' or 's'
char softSTRHeader[11] = {'p', 'm', '.', 's', '0', '.', 't', 'x', 't', '=', '\"'};
char softINTHeader[10] = {'p', 'm', '.', 'v', '0', '.', 'v', 'a', 'l', '='};

char softTemp[20];

const uint8_t ResponseHeader[11]={'p', 'm', '.', 's', 'p', '.', 't', 'x', 't', '=', '"'}; //for Spectrum from DSP
const uint8_t ResponseFooter[4]={'"', 0xFF, 0xFF, 0xFF};
const char HexCodes[16] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', };

void FFT(double *x,double *y, int n, long m);

double FFTReal[SAMPLESIZE];
double FFTImag[SAMPLESIZE];
int ADC_MAX   = 0;
int ADC_MIN   = 0;
int ADC_DIFF  = 0;
unsigned long SAMPLE_INTERVAL = 0;

char nowADCSampling = 0;  //prevent for loss signal

//===================================================================
//Begin of Nextion LCD Protocol
//
// v0~v9, va~vz : Numeric (Transceiver -> Nextion LCD)
// s0~s9  : String (Text) (Transceiver -> Nextion LCD)
// vlSendxxx, vloxxx: Reserve for Nextion (Nextion LCD -> Transceiver)
//
//===================================================================
#define CMD_NOW_DISP      '0' //c0
char L_nowdisp = -1;          //Sended nowdisp

#define CMD_VFO_TYPE      'v' //cv
char L_vfoActive;             //vfoActive

#define CMD_CURR_FREQ     'c' //vc
unsigned long L_vfoCurr;      //vfoA
#define CMD_CURR_MODE     'c' //cc
byte L_vfoCurr_mode;          //vfoA_mode

#define CMD_VFOA_FREQ     'a' //va
unsigned long L_vfoA;         //vfoA
#define CMD_VFOA_MODE     'a' //ca
byte L_vfoA_mode;             //vfoA_mode

#define CMD_VFOB_FREQ     'b' //vb
unsigned long L_vfoB;         //vfoB
#define CMD_VFOB_MODE     'b' //cb
byte L_vfoB_mode;             //vfoB_mode

#define CMD_IS_RIT        'r' //cr
char L_ritOn;
#define CMD_RIT_FREQ      'r' //vr
unsigned long L_ritTxFrequency; //ritTxFrequency

#define CMD_IS_TX         't' //ct
char L_inTx;

#define CMD_IS_DIALLOCK   'l' //cl
byte L_isDialLock;            //byte isDialLock

#define CMD_IS_SPLIT      's' //cs
byte  L_Split;            //isTxType
#define CMD_IS_TXSTOP     'x' //cx
byte  L_TXStop;           //isTxType

#define CMD_TUNEINDEX     'n' //cn
byte L_tuneStepIndex;     //byte tuneStepIndex

#define CMD_SMETER        'p' //cs
byte L_scaledSMeter;      //scaledSMeter

#define CMD_SIDE_TONE     't' //vt
unsigned long L_sideTone; //sideTone
#define CMD_KEY_TYPE      'k' //ck
byte L_cwKeyType;          //L_cwKeyType 0: straight, 1 : iambica, 2: iambicb

#define CMD_CW_SPEED      's' //vs
unsigned int L_cwSpeed;   //cwSpeed

#define CMD_CW_DELAY      'y' //vy
byte L_cwDelayTime;       //cwDelayTime

#define CMD_CW_STARTDELAY 'e' //ve
byte L_delayBeforeCWStartTime;  //byte delayBeforeCWStartTime

#define CMD_ATT_LEVEL     'f' //vf
byte L_attLevel;

byte L_isIFShift;             //1 = ifShift, 2 extend
#define CMD_IS_IFSHIFT    'i' //ci

int L_ifShiftValue;
#define CMD_IFSHIFT_VALUE 'i' //vi

byte L_sdrModeOn;
#define CMD_SDR_MODE      'j' //cj

#define CMD_UBITX_INFO     'm' //cm  Complete Send uBITX Information

//Once Send Data, When boot
//arTuneStep, When boot, once send
//long arTuneStep[5];
#define CMD_AR_TUNE1      '1' //v1
#define CMD_AR_TUNE2      '2' //v2
#define CMD_AR_TUNE3      '3' //v3
#define CMD_AR_TUNE4      '4' //v4
#define CMD_AR_TUNE5      '5' //v5

//int idleStep = 0;
byte scaledSMeter = 0;

//send data for Nextion LCD
void SendHeader(char varType, char varIndex)
{
  if (varType == SWS_HEADER_STR_TYPE)
  {
    softSTRHeader[4] = varIndex;
    for (int i = 0; i < 11; i++)
      SWSerial_Write(softSTRHeader[i]);
  }
  else
  {
    softINTHeader[4] = varIndex;
    for (int i = 0; i < 10; i++)
      SWSerial_Write(softINTHeader[i]);
  }
}

void SendCommandUL(char varIndex, unsigned long sendValue)
{
  SendHeader(SWS_HEADER_INT_TYPE, varIndex);

  memset(softTemp, 0, 20);
  ultoa(sendValue, softTemp, DEC);
  SWSerial_Print((uint8_t*) softTemp);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);  
}

void SendCommandL(char varIndex, long sendValue)
{
  SendHeader(SWS_HEADER_INT_TYPE, varIndex);

  memset(softTemp, 0, 20);
  ltoa(sendValue, softTemp, DEC);
  SWSerial_Print((uint8_t*) softTemp);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);
  SWSerial_Write(0xff);  
}

void SendCommandStr(char varIndex, char* sendValue)
{
  SendHeader(SWS_HEADER_STR_TYPE, varIndex);
  
  SWSerial_Print((uint8_t*) sendValue);
  SWSerial_Write('\"');
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
  SWSerial_Write(0xFF);
}

char softBuff1Num[14] = {'p', 'm', '.', 'c', '0', '.', 'v', 'a', 'l', '=', 0, 0xFF, 0xFF, 0xFF};
void SendCommand1Num(char varType, char sendValue) //0~9 : Mode, nowDisp, ActiveVFO, IsDialLock, IsTxtType, IsSplitType
{
  softBuff1Num[4] = varType;
  softBuff1Num[10] = sendValue + 0x30;

  for (int i = 0; i < 14; i++)
    SWSerial_Write(softBuff1Num[i]);
}

//=======================================================
//END OF Nextion Protocol
//=======================================================

int I2CCommand = 0;
void CalculateCoeff(uint8_t freqIndex);

char ForwardBuff[MAX_FORWARD_BUFF_LENGTH + 1];
static char nowBuffIndex = 0;
static char etxCount = 0;
static char nowSendingProtocol = 0;

uint8_t SMeterToUartSend       = 0; //0 : Send, 1: Idle
uint8_t SMeterToUartIdleCount  = 0;
#define SMeterToUartInterval  4

char DSPType = 1; //0 : Not Use, 1 : FFT, 2 : Morse Decoder, 3 : RTTY Decoder
char FFTToUartIdleCount = 0;
#define FFTToUartInterval 2

unsigned long lastForwardmili = 0;
uint8_t responseCommand = 0;  //
uint8_t TXStatus = 0;   //0:RX, 1:TX
void ResponseConfig()
{
  if (responseCommand == 2)
  {
    unsigned long returnValue = 0;
    if (DSPType == 0)
    {
      returnValue = 94; //None
    }
    else if (DSPType == 1)
    {
      returnValue = 95; //Spectrum (FFT) mode
    }
    else if (DSPType == 2)
    {
      returnValue = 100 + cwDecodeHz;
    }
    
    returnValue = returnValue << 8;
    returnValue = returnValue |  (SMeterToUartSend & 0xFF);
    returnValue = returnValue << 8;
    uint8_t tmpValue = 0;
    if (magnitudelimit_low > 255)
      tmpValue = 255;
    else if (magnitudelimit_low < 1)
      tmpValue = 0;
    else 
      tmpValue = magnitudelimit_low;
    returnValue = returnValue |  (tmpValue & 0xFF);
    
    SendCommandUL('v', returnValue);     //Return data  
    SendCommandUL('g', 0x6A);     //Return data  
  }
  responseCommand = 0;
}

//Result : if found .val=, 1 else 0
char CommandPasrser(int lastIndex)
{
  //Analysing Forwrd data
  //59 58 68 4A   1C 5F 6A E5     FF FF 73 
  //Find Loopback protocol
  // 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19    20 21 22
  //70 6D 2E 76 76 2E 76 61 6C 3D 33 38 34 38 39 35 33 36 32 38    FF FF FF 
  //pm.vv.val=3848953628\xFF\xFF\xFF
  //1234567890XXX
  //
  int startIndex = 0;

  //Loop back command has 13 ~ 23
  if (lastIndex < 13)
  {
    return 0;
  }

  //Protocol MAX Length : 22
  if (lastIndex >= 22)
  {
    startIndex = lastIndex - 22;
  }
  else
  {
    startIndex = 0;
  }

  for (int i = lastIndex - 3; i >= startIndex + 7; i--)
  {
    //Find =
    if (ForwardBuff[i-3] == 'v' && ForwardBuff[i-2] == 'a' && ForwardBuff[i-1] == 'l' && ForwardBuff[i] == '=')  //0x3D
    {
      uint8_t command1 = ForwardBuff[i-6];  //v
      uint8_t command2 = ForwardBuff[i-5];  //v
                              //     i-4    //.
      ForwardBuff[lastIndex - 2] = 0;
      long commandVal=atol(&ForwardBuff[i + 1]);
      uint8_t *ReadBuff = (uint8_t *)&commandVal;
                              
      //Loop Back
      if (command1 == 'v' && command2 == 'v')
      {
        //===========================================================
        //Test Code 1
        /*
        long tmpVal = 0;
        Serial.print("Found :");
        for (int k = i + 1; k <= lastIndex - 3; k++)
        {
          Serial.write(ForwardBuff[k]);
        }
        Serial.println();
        Serial.print("Reverse :");
        for (int k = lastIndex - 3; k >= i + 1; k--)
        {
          Serial.write(ForwardBuff[k]);
        }
        Serial.println();

        ForwardBuff[lastIndex - 2] = 0;

        tmpVal=atol(&ForwardBuff[i + 1]);
        Serial.println(tmpVal);

        uint8_t *ReadBuff = (uint8_t *)&tmpVal;
        char buff[10];
        sprintf(buff, "%x,%x,%x,%x", ReadBuff[0], ReadBuff[1], ReadBuff[2], ReadBuff[3]);
        Serial.println(buff);

        int calcChecksumA = ReadBuff[0] + ReadBuff[1] + ReadBuff[2];
        calcChecksumA = calcChecksumA % 256;

        if (calcChecksumA == ReadBuff[3])
        {
          Serial.print("Correct Checksum : ");
          Serial.print(ReadBuff[3]);
          Serial.print(":");
          Serial.println(calcChecksumA);
        }
        //End of Test Code
        */

        /*
        //Test Code 2
         Serial.print("Found :");
         Serial.print(startIndex);
         Serial.print(",");
         Serial.print(lastIndex);
         Serial.print(":");
        for (int k = i + 1; k <= lastIndex - 3; k++)
        {
          Serial.write(ForwardBuff[k]);
        }
        Serial.println("");
        //End of Tet Code 2
        */

        int calcChecksum = ReadBuff[0] + ReadBuff[1] + ReadBuff[2];
        calcChecksum = calcChecksum % 256;

        //Correct Checksum and Receiver is DSP Moudle protocol v1.0
        if (calcChecksum == ReadBuff[3] && ReadBuff[2] == 0x6A)
        {
          //Serial.print("Correct Checksum Command : ");
          //Serial.println(ReadBuff[1]);
          uint8_t cmd1 = ReadBuff[1];
          if (cmd1 == 94)
          {
            DSPType = 0;
            EEPROM.put(EEPROM_DSPTYPE, DSPType);
          }
          else if (cmd1 == 95)
          {
            //Serial.println("Spectrum Mode");
            DSPType = 1;
            EEPROM.put(EEPROM_DSPTYPE, DSPType);
          }
          else if (cmd1 >= 100 && cmd1 <= 145)
          {
            cwDecodeHz = cmd1 - 100;
            CalculateCoeff(cwDecodeHz);
            DSPType = 2;
            EEPROM.put(EEPROM_DSPTYPE, DSPType);
            EEPROM.put(EEPROM_CW_FREQ, cwDecodeHz);
          }
          else if (cmd1 > 1 && cmd1 <= 5) //2~5 : Request Configuration
          {
            responseCommand = cmd1;
          }
          else if (cmd1 == 50 || cmd1 == 51) //Set Configuration
          {
            SMeterToUartSend = (cmd1 == 51);
            EEPROM.put(EEPROM_SMETER_UART, SMeterToUartSend);
          }
          else if (cmd1 >= 146 && cmd1 <= 156 )
          {
            //Save Mode
            magnitudelimit_low = (cmd1 - 146) * 10;
            EEPROM.put(EEPROM_CW_MAG_LOW, magnitudelimit_low);
          } //end of if
        } //end of check Checksum
      }   //end of check Protocol (vv)
      else if (command1 == 'c' && command2 == 't')  //TX, RX
      {
        if (commandVal == 0)  //RX
        {
          TXStatus = 0;
          SMeterToUartIdleCount = 0;
        }
        else if (commandVal == 1) //TX
        {
          TXStatus = 1;
          SMeterToUartIdleCount = 0;
        }
      }

      return 1;
    }     //end of check Protocol (.val)
  }       //end of for

  //Not found Protocol (.val=
  return 0;
}

//#define PROTOCOL_TIMEOUT = 100
int ForwardData(void)
{
   uint8_t recvChar;

  if (Serial.available() > 0)
  {
#ifndef USE_SW_SERIAL
    Serial.flush();
#endif
    //Check RX Buffer
    while (Serial.available() > 0)
    {
      recvChar = Serial.read();

      ForwardBuff[nowBuffIndex] = recvChar;

      if (recvChar == 0xFF)       //found ETX
      {
        etxCount++;               //Nextion Protocol, ETX : 0xFF, 0xFF, 0xFF
        if (etxCount >= 3)
        {
          //Finished Protocol
          if (CommandPasrser(nowBuffIndex) == 1)
          {
            nowSendingProtocol = 0; //Finished 1 Set Command
            etxCount = 0;
            nowBuffIndex = 0;
          }
        }
      }
      else
      {
        etxCount = 0x00;
        nowSendingProtocol = 1; //Sending Data
      }
      
      SWSerial_Write(recvChar);
      lastForwardmili = millis();
      nowBuffIndex++;

      if (nowBuffIndex > MAX_FORWARD_BUFF_LENGTH -2)
      {
        nowBuffIndex = 0;
      }
    } //end of while

#ifndef USE_SW_SERIAL
    Serial.flush();
#endif

    //lastReceivedTime = millis();
  } //end if (Serial.available
  else
  {
      //check Timeout
    
  }
}

/*
int SendMeterData(uint8_t isSend)
{
  int newScaledSMeter = 0;
  if (ADC_DIFF > 11)
  {
    newScaledSMeter = 8;
  }
  else if (ADC_DIFF > 9)
  {
    newScaledSMeter = 7;
  }
  else if (ADC_DIFF > 8)
  {
    newScaledSMeter = 6;
  }
  else if (ADC_DIFF > 7)
  {
    newScaledSMeter = 5;
  }
  else if (ADC_DIFF > 6)
  {
    newScaledSMeter = 4;
  }
  else if (ADC_DIFF > 5)
  {
    newScaledSMeter = 3;
  }
  else if (ADC_DIFF > 4)
  {
    newScaledSMeter = 2;
  }
  else if (ADC_DIFF > 3)
  {
    newScaledSMeter = 1;
  }
  else
  {
    newScaledSMeter = 0;
  }
  scaledSMeter = newScaledSMeter;

  if (isSend == 1)
  {
    if (L_scaledSMeter != scaledSMeter)
    {
      L_scaledSMeter = scaledSMeter;
      SendCommand1Num(CMD_SMETER, L_scaledSMeter);  
    }
  }
}
*/
int SendMeterData(uint8_t isSend)
{
  //basic : 1.5Khz
  int newScaledSMeter = 0;
  //if (ADC_DIFF > 26)  //-63dBm : S9 + 10dBm
  if (ADC_DIFF > 26)  //-63dBm : S9 + 10dBm  (subtract loss rate)
  //if (ADC_DIFF > 55)  //-63dBm : S9 + 15dBm
  {
    newScaledSMeter = 8;
  }
  else if (ADC_DIFF > 11) //~ -72  S9
  {
    newScaledSMeter = 7;
  }
  else if (ADC_DIFF > 8)
  {
    newScaledSMeter = 6;
  }
  else if (ADC_DIFF > 6)  //~-80 S7  
  //else if (ADC_DIFF > 5)  //~-80 S7  
  {
    newScaledSMeter = 5;    
  }
  else if (ADC_DIFF > 4)  
  {
    newScaledSMeter = 4;    //79   S8
  }
  else if (ADC_DIFF > 3) 
  {
    newScaledSMeter = 3;   //-81 ~ -78 => S7
  }
  else if (ADC_DIFF > 2)  
  {
    newScaledSMeter = 2; // -88 ~ -82 => S7 or S6
  }
  else if (ADC_DIFF > 1)  //-93 ~ -89 => S5 or S4
  {
    newScaledSMeter = 1;
  }
  else    // ~ -93.0dBm ~S3 or S4
  {
    newScaledSMeter = 0;
  }

/*
1 : with noise (not use 0 ~ S3) 
2 : -93 ~ -89
3 : -88 ~ -81
4 : -80 ~ -78
5 : -77 ~ -72
6 : -71 ~ -69
 */
  
  scaledSMeter = newScaledSMeter;

  if (isSend == 1)
  {
    if (L_scaledSMeter != scaledSMeter)
    {
      L_scaledSMeter = scaledSMeter;
      SendCommand1Num(CMD_SMETER, L_scaledSMeter);  
    }
  }
}


void GrepADC(void)
{
  int readedValue = 0;
  unsigned long currentms = 0;
  int readSampleCount = 0;

  if (DSPType == 2 || DSPType == 0) //Decode Morse
  {
    readSampleCount = DECODE_MORSE_SAMPLESIZE;
  }
  else if (DSPType == 3) //Decode RTTY
  {
    readSampleCount = DECODE_MORSE_SAMPLESIZE;
  }
  else
  {
    readSampleCount = SAMPLESIZE;
  }
  
  ADC_MAX = 0;
  ADC_MIN = 30000;
  
  for(int i=0; i < readSampleCount; i++)
  {
    currentms = micros();
    readedValue = analogRead(SIGNAL_METER_ADC);;
    FFTReal[i] = readedValue;
    FFTImag[i] = 0;
    
    if (ADC_MAX < readedValue)
    {
      ADC_MAX = readedValue;
    }
    
    if (ADC_MIN > readedValue)
    {
      ADC_MIN = readedValue;
    }
    
    while(micros() < (currentms + SAMPLE_INTERVAL)){}
  } //end of for
}

void SendFFTData(void)
{
  int readedValue = 0;
    for (int i = 0; i < 11; i++)
      SWSerial_Write(ResponseHeader[i]);

    for(int i = 1; i < 64; i++)
    {
      readedValue = (int)(FFTReal[i]);
      if (readedValue < 0)
      {
        readedValue = 0;
      }
      else if (readedValue>255)
      {
        readedValue=255;
      }
      SWSerial_Write(HexCodes[readedValue >> 4]);
      SWSerial_Write(HexCodes[readedValue & 0xf]);
    }

  for (int i = 0; i < 4; i++)
    SWSerial_Write(ResponseFooter[i]);
}

void i2cMeterSetup() 
{
  //Load Configuration
  EEPROM.get(EEPROM_DSPTYPE, DSPType);
  if (DSPType > 5)
  {
    DSPType = 1;
  }

  //Signal Meter
  EEPROM.get(EEPROM_SMETER_UART, SMeterToUartSend);
  if (SMeterToUartSend > 2)
  {
    SMeterToUartSend = 1;
  }
  //
  EEPROM.get(EEPROM_CW_FREQ, cwDecodeHz);
  if (cwDecodeHz > 40 || cwDecodeHz < 1)
  {
    cwDecodeHz = 9;
  }

  //EEPROM_CW_MAG_LOW
  EEPROM.get(EEPROM_CW_MAG_LOW, magnitudelimit_low);
  if (magnitudelimit_low > 1000 || magnitudelimit_low < 1)
  {
    magnitudelimit_low = 50;
  }
  
  // put your setup code here, to run once:
  Wire.begin(I2CMETER_ADDR);              //j : S-Meter Slave Address
  //Wire.begin(0x21);                     //j : S-Meter Slave Address
  Wire.onReceive(I2CReceiveEvent);        //
  Wire.onRequest(I2CRequestEvent);
  
#ifdef USE_SW_SERIAL
  SWSerial_Begin(9600);
#endif  

  Serial.begin(9600, SERIAL_8N1);
  Serial.flush();
  SAMPLE_INTERVAL = round(1000000 * (1.0 / SAMPLE_PREQUENCY));
  CalculateCoeff(cwDecodeHz);  //Set 750Hz //9 * 50 + 300 = 750Hz
  //Serial.println("Start...");
}

void I2CReceiveEvent(int howMany)
{
  int readCommand = 0; // byte를 읽어 int로 변환  
  
  while(Wire.available() > 0)     // for Last command
  {
     readCommand = Wire.read();
  }

  if (0x50 <= readCommand && readCommand <= 0x59)
  {
    I2CCommand = readCommand;
  }
}

void I2CRequestEvent(void)
{
  int maxValue = 0;
  int minValue = 30000;
  int readedValue = 0;
  unsigned long curr = 0;

  //if (nowADCSampling == 1)  //Now Sampling ADC's
  //{
  //  nowADCSampling = 2;     //when finished ADC Sampling, response I2CRequest Event
  //  return;
  //} //end of if
  
  if (I2CCommand == I2CMETER_CALCS)
  {
    Wire.write(scaledSMeter);
  }
  else if (I2CCommand == I2CMETER_UNCALCS)
  {
    //Wire.write(ADC_DIFF);
    //8292Hz
    for(int i=0; i < 7; i++)
    {
      curr = micros();
      readedValue = analogRead(SIGNAL_METER_ADC);;

      if (readedValue > maxValue)
      {
        maxValue = readedValue;
      }

      if (readedValue < minValue)
      {
        minValue = readedValue;
      }
      while(micros() < (curr + 127)){} //8Khz / 7
    } //end of for

    readedValue = maxValue - minValue;
    readedValue = readedValue * readedValue;
    //readedValue = readedValue / 2;
    if (readedValue < 0)
    {
      readedValue = 0;
    }
    else if (readedValue > 255)
    {
      readedValue = 255;
    }
    Wire.write(readedValue);
  }
  else if (I2CCommand == I2CMETER_CALCP)
  {
    readedValue = analogRead(POWER_METER_ADC);  //POWER
    Wire.write(readedValue);
  }
  else if (I2CCommand == I2CMETER_CALCR)        //SWR
  {
    readedValue = analogRead(SWR_METER_ADC);
    Wire.write(readedValue);
  }
}

extern void Decode_Morse(float magnitude);
extern double coeff;


#define LAST_TIME_INTERVAL 159

int SWRAdcValue = 0;

//for boot Delay, a alot of data transfer
//Delay 2.5 Sec
byte isBooted = 0;

void i2cMeterLoop() 
{
  char isProcess = 0; //0 : Init, 1 : Complete ADC Sampling, 2 : Complete FFT
  isProcess = 0;

  ForwardData();
  if (isBooted < 100)
  {
    //Delay 20msec
    for (int i = 0; i < 20; i++)
    {
      ForwardData();
      delay(1);
    }
    isBooted++;
    return;
  }

  //===========================================
  //TRANSCEIVER STATUS : RX
  //===========================================
  if (TXStatus == 1)  //TX Mode
  {
    int readedValue = 0;
    SMeterToUartIdleCount++;
    if (SMeterToUartIdleCount > 130)          //SWR  
    {
      //SWR Send
      SendCommandL('m', SWRAdcValue);
      SendCommand1Num('m', 3);
      SMeterToUartIdleCount = 0;
    }
    else if (SMeterToUartIdleCount == 100)  //POWER 500msec interval
    {
      readedValue = analogRead(POWER_METER_ADC   );
      SWRAdcValue = analogRead(SWR_METER_ADC);
      SendCommandL('m', readedValue);
      SendCommand1Num('m',2);
      //PWR Send
    }
    ForwardData();
    delay(5);
    
    return; //Do not processing ADC, FFT, Decode Morse or RTTY, only Power and SWR Data Return
  }

  //===========================================
  //TRANSCEIVER STATUS : RX
  //===========================================
  //===========================================
  // ADC Sampling
  //===========================================
  if (nowSendingProtocol == 0)  //Idle Status
  {
    nowADCSampling = 1;         //Mark => Start Sampling
    GrepADC();
    
    //if(nowADCSampling == 2)     //Marked ? While ADC Sampling, receive I2C
    //{
    //  nowADCSampling = 0;       //Mark => Finish Sampling
    //  I2CRequestEvent();
    //}
    nowADCSampling = 0;         //Mark => Finish Sampling

    int newDiff = ADC_MAX - ADC_MIN; //Calculated for Signal Meter (ADC_DIFF => Signal Strength)
    //ADC_DIFF = ((ADC_DIFF * 7) + (newDiff * 3)) / 10;
    ADC_DIFF = newDiff;
    isProcess = 1;              //Mark => Complete ADC Sampling
  }

  ForwardData();

  //===========================================
  // Send Signal Meter to UART
  //===========================================
  if (SMeterToUartSend == 1)        //SMeter To Uart Send
  {
    //When selected Morse decode mode, not send signal Meter
    //if ((DSPType != 2) && (SMeterToUartIdleCount++ > (SMeterToUartInterval * (DSPType == 1 ? 1 : 12))))
    //User Option
    if (SMeterToUartIdleCount++ > (SMeterToUartInterval * (DSPType == 1 ? 1 : 12)))
    {
      //nowSendingProtocol -> not finished data forward, (not found 0xff, 0xff, 0xff yet)
      if (nowSendingProtocol == 0 && isProcess == 1) //Complete ADC Sampling and Idle status
      {
        SendMeterData(1);
        SMeterToUartIdleCount = 0;
      }
    }
  } //end of if
  else
  {
    SendMeterData(0); //only calculate Signal Level
  }
  
  ForwardData();

  //Check Response Command
  if (responseCommand > 0 && millis() > lastForwardmili + LAST_TIME_INTERVAL)
  {
    ResponseConfig();
  }

  //===================================================================================
  // DSP Routine
  //===================================================================================
  if (DSPType == 1 && millis() > lastForwardmili + LAST_TIME_INTERVAL) //Spectrum : FFT => Send To UART
  {
    FFTToUartIdleCount = 0;
    
    if (isProcess == 1)
    {
      FFT(FFTReal, FFTImag, SAMPLESIZE, 7);
      isProcess = 2;
    }
    
    ForwardData();
  
    if (isProcess == 2)
    {
      for (uint16_t k = 0; k < SAMPLESIZE; k++) 
      {
        FFTReal[k] = sqrt(FFTReal[k] * FFTReal[k] + FFTImag[k] * FFTImag[k]);
      }
  
      isProcess = 3;
    }
    
    ForwardData();
  
    if (isProcess == 3)
    {
      if (nowSendingProtocol == 0)  //Idle Status
      {
        SendFFTData();
      }
    }
  }
  else if (DSPType == 2)          //Decode Morse
  {
    //Implement Goertzel_algorithm
    //https://en.wikipedia.org/wiki/Goertzel_algorithm

    /*
    ω = 2 * π * Kterm / Nterms;
    cr = cos(ω);
    ci = sin(ω);
    coeff = 2 * cr;
    
    sprev = 0;
    sprev2 = 0;
    for each index n in range 0 to Nterms-1
      s = x[n] + coeff * sprev - sprev2;
      sprev2 = sprev;
      sprev = s;
    end
    
    power = sprev2 * sprev2 + sprev * sprev - coeff * sprev * sprev2;
     */
    double Q1 = 0;
    double Q2 = 0;
    
    for (char index = 0; index < DECODE_MORSE_SAMPLESIZE; index++)
    {
      float Q0;
      Q0 = coeff * Q1 - Q2 + FFTReal[index];
      Q2 = Q1;
      Q1 = Q0;
    }
    double magnitudeSquared = (Q1*Q1)+(Q2*Q2)-Q1*Q2*coeff;  // we do only need the real part //
    double magnitude = sqrt(magnitudeSquared);
    
    Decode_Morse(magnitude);
  } //enf of if 
} //end of main



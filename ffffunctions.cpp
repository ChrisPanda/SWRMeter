/*
FFTFunctions for Nextion LCD and Control MCU
This code is for FFT and CW Decode. 
KD8CEC, Ian Lee
-----------------------------------------------------------------------
//The section on CW decode logic is specified at the bottom of this code.
License : I follow the license of the previous code and I do not add any extra constraints.
I hope that the Comment I made or the Comment of OZ1JHM will be maintained.
**********************************************************************/
#include "define.h"

#ifdef I2C_H

#include <arduino.h>
#include "i2cmeter.h"

// Code Referency : http://paulbourke.net/miscellaneous/dft/
// DFT, FFT Wiritten by Paul Bourke, June 1993
void FFT(double *x,double *y, int n, long m)
{
   long i,i1,j,k,i2,l,l1,l2;
   double c1,c2,tx,ty,t1,t2,u1,u2,z;
   short int dir = 0;

   /* Do the bit reversal */
   i2 = n >> 1;
   j = 0;
   for (i=0;i<n-1;i++) {
      if (i < j) {
         tx = x[i];
         ty = y[i];
         x[i] = x[j];
         y[i] = y[j];
         x[j] = tx;
         y[j] = ty;
      }
      k = i2;
      while (k <= j) {
         j -= k;
         k >>= 1;
      }
      j += k;
   }

   /* Compute the FFT */
   c1 = -1.0; 
   c2 = 0.0;
   l2 = 1;
   
   for (l=0;l<m;l++) 
   {
      l1 = l2;
      l2 <<= 1;
      u1 = 1.0; 
      u2 = 0.0;
      for (j=0;j<l1;j++) 
      {
         for (i=j;i<n;i+=l2) 
         {
            i1 = i + l1;
            t1 = u1 * x[i1] - u2 * y[i1];
            t2 = u1 * y[i1] + u2 * x[i1];
            x[i1] = x[i] - t1; 
            y[i1] = y[i] - t2;
            x[i] += t1;
            y[i] += t2;
         }
         z =  u1 * c1 - u2 * c2;
         u2 = u1 * c2 + u2 * c1;
         u1 = z;
      }
      c2 = sqrt((1.0 - c1) / 2.0);
      if (dir == 1) 
         c2 = -c2;
      c1 = sqrt((1.0 + c1) / 2.0);
   }

   /* Scaling for forward transform */
   /*
   if (dir == 1) {
      for (i=0;i<n;i++) {
         x[i] /= n;
         y[i] /= n;
      }
   }

   return 1;
   */
   
   //return(TRUE);
}

double coeff;

void CalculateCoeff(uint8_t freqIndex)
{
  float omega;
  int targetFrequency = freqIndex * 50 + 300;
  int k = (int) (0.5 + ((DECODE_MORSE_SAMPLESIZE * targetFrequency) / SAMPLE_PREQUENCY));
  omega = (2.0 * PI * k) / DECODE_MORSE_SAMPLESIZE;
  coeff = 2.0 * cos(omega);
}

//=====================================================================
//The CW Decode code refers to the site code below.
//https://k2jji.org/2014/09/18/arduino-base-cw-decoder/
//Some code has been modified, but the original comments remain intact.
// code below is optimal for use in Arduino.
//Thanks to OZ1JHM 
//KD8CEC
//=====================================================================

///////////////////////////////////////////////////////////////////////
// CW Decoder made by Hjalmar Skovholm Hansen OZ1JHM  VER 1.01       //
// Feel free to change, copy or what ever you like but respect       //
// that license is http://www.gnu.org/copyleft/gpl.html              //
// Discuss and give great ideas on                                   //
// https://groups.yahoo.com/neo/groups/oz1jhm/conversations/messages //
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////
// Read more here http://en.wikipedia.org/wiki/Goertzel_algorithm        //
// if you want to know about FFT the http://www.dspguide.com/pdfbook.htm //
///////////////////////////////////////////////////////////////////////////

//int magnitudelimit = 50;
//int magnitudelimit_low = 50;
int magnitudelimit = 30;
int magnitudelimit_low = 30;
char realstate = LOW;
char realstatebefore = LOW;
char filteredstate = LOW;
char filteredstatebefore = LOW;
long laststarttime = 0;
int nbtime = 6;  /// ms noise blanker         

long starttimehigh;
long highduration;
long lasthighduration;
long hightimesavg;
long lowtimesavg;
long startttimelow;
long lowduration;

char code[20];
uint8_t stop = LOW;
int wpm;
uint8_t cwDecodeHz = 9;

extern void SendCommandStr(char varIndex, char* sendValue);

void printascii(int asciinumber)
{
  char rstDecode[4] = {0, 0, 0, 0};
  
  if (asciinumber == 3)
  {
    
  }
  else if (asciinumber == 4)
  {
    
  }
  else if (asciinumber == 6)
  {
    
  }
  else
  {
    rstDecode[0] = asciinumber;
  }
  SendCommandStr('b',   rstDecode);
  
  //Serial.write(asciinumber);
  //if (writeCount++ > 20)
  //{
    //writeCount = 0;
    //Serial.println("");
  //}
}

uint8_t docode()
{
  if (strcmp(code,".-") == 0) printascii(65);
  if (strcmp(code,"-...") == 0) printascii(66);
  if (strcmp(code,"-.-.") == 0) printascii(67);
  if (strcmp(code,"-..") == 0) printascii(68);
  if (strcmp(code,".") == 0) printascii(69);
  if (strcmp(code,"..-.") == 0) printascii(70);
  if (strcmp(code,"--.") == 0) printascii(71);
  if (strcmp(code,"....") == 0) printascii(72);
  if (strcmp(code,"..") == 0) printascii(73);
  if (strcmp(code,".---") == 0) printascii(74);
  if (strcmp(code,"-.-") == 0) printascii(75);
  if (strcmp(code,".-..") == 0) printascii(76);
  if (strcmp(code,"--") == 0) printascii(77);
  if (strcmp(code,"-.") == 0) printascii(78);
  if (strcmp(code,"---") == 0) printascii(79);
  if (strcmp(code,".--.") == 0) printascii(80);
  if (strcmp(code,"--.-") == 0) printascii(81);
  if (strcmp(code,".-.") == 0) printascii(82);
  if (strcmp(code,"...") == 0) printascii(83);
  if (strcmp(code,"-") == 0) printascii(84);
  if (strcmp(code,"..-") == 0) printascii(85);
  if (strcmp(code,"...-") == 0) printascii(86);
  if (strcmp(code,".--") == 0) printascii(87);
  if (strcmp(code,"-..-") == 0) printascii(88);
  if (strcmp(code,"-.--") == 0) printascii(89);
  if (strcmp(code,"--..") == 0) printascii(90);

  if (strcmp(code,".----") == 0) printascii(49);
  if (strcmp(code,"..---") == 0) printascii(50);
  if (strcmp(code,"...--") == 0) printascii(51);
  if (strcmp(code,"....-") == 0) printascii(52);
  if (strcmp(code,".....") == 0) printascii(53);
  if (strcmp(code,"-....") == 0) printascii(54);
  if (strcmp(code,"--...") == 0) printascii(55);
  if (strcmp(code,"---..") == 0) printascii(56);
  if (strcmp(code,"----.") == 0) printascii(57);
  if (strcmp(code,"-----") == 0) printascii(48);

  if (strcmp(code,"..--..") == 0) printascii(63);
  if (strcmp(code,".-.-.-") == 0) printascii(46);
  if (strcmp(code,"--..--") == 0) printascii(44);
  if (strcmp(code,"-.-.--") == 0) printascii(33);
  if (strcmp(code,".--.-.") == 0) printascii(64);
  if (strcmp(code,"---...") == 0) printascii(58);
  if (strcmp(code,"-....-") == 0) printascii(45);
  if (strcmp(code,"-..-.") == 0) printascii(47);

  if (strcmp(code,"-.--.") == 0) printascii(40);
  if (strcmp(code,"-.--.-") == 0) printascii(41);
  if (strcmp(code,".-...") == 0) printascii(95);
  if (strcmp(code,"...-..-") == 0) printascii(36);
  if (strcmp(code,"...-.-") == 0) printascii(62);
  if (strcmp(code,".-.-.") == 0) printascii(60);
  if (strcmp(code,"...-.") == 0) printascii(126);
  //////////////////
  // The specials //
  //////////////////
  if (strcmp(code,".-.-") == 0) printascii(3);
  if (strcmp(code,"---.") == 0) printascii(4);
  if (strcmp(code,".--.-") == 0) printascii(6);

}

void Decode_Morse(float magnitude)
{
  //magnitudelimit auto Increase
  if (magnitude > magnitudelimit_low)
  {
    magnitudelimit = (magnitudelimit +((magnitude - magnitudelimit)/6));  /// moving average filter
  }

  if (magnitudelimit < magnitudelimit_low)
    magnitudelimit = magnitudelimit_low;
  
  if(magnitude > magnitudelimit*0.6) // just to have some space up 
     realstate = HIGH; 
  else
    realstate = LOW; 

  if (realstate != realstatebefore)
    laststarttime = millis();

  if ((millis()-laststarttime) > nbtime)
  {
    if (realstate != filteredstate)
    {
      filteredstate = realstate;
    }
  }

  if (filteredstate != filteredstatebefore)
  {
    if (filteredstate == HIGH)
    {
      starttimehigh = millis();
      lowduration = (millis() - startttimelow);
    }
    
    if (filteredstate == LOW)
    {
      startttimelow = millis();
      highduration = (millis() - starttimehigh);
      
      if (highduration < (2*hightimesavg) || hightimesavg == 0)
      {
        hightimesavg = (highduration+hightimesavg+hightimesavg)/3;     // now we know avg dit time ( rolling 3 avg)
      }
      
      if (highduration > (5*hightimesavg) )
      {
        hightimesavg = highduration+hightimesavg;     // if speed decrease fast ..
      }
    }
  }

 ///////////////////////////////////////////////////////////////
 // now we will check which kind of baud we have - dit or dah //
 // and what kind of pause we do have 1 - 3 or 7 pause        //
 // we think that hightimeavg = 1 bit                         //
 ///////////////////////////////////////////////////////////////
 
 if (filteredstate != filteredstatebefore)
 {
    stop = LOW;
    if (filteredstate == LOW)
    {
      if (highduration < (hightimesavg*2) && highduration > (hightimesavg*0.6))  /// 0.6 filter out false dits
      {
        strcat(code,".");
      }
      if (highduration > (hightimesavg*2) && highduration < (hightimesavg*6))
      {
        strcat(code,"-");
        wpm = (wpm + (1200/((highduration)/3)))/2;  //// the most precise we can do ;o)
      }
    }
 
    if (filteredstate == HIGH)
    {
      float lacktime = 1;
      if(wpm > 25)lacktime=1.0; ///  when high speeds we have to have a little more pause before new letter or new word 
      if(wpm > 30)lacktime=1.2;
      if(wpm > 35)lacktime=1.5;
      
      if (lowduration > (hightimesavg*(2*lacktime)) && lowduration < hightimesavg*(5*lacktime))  // letter space
      {
        docode();
        code[0] = '\0';
      }
      if (lowduration >= hightimesavg*(5*lacktime))
      { // word space
        docode();
        code[0] = '\0';
        printascii(32);
      }
    }
 }

  if ((millis() - startttimelow) > (highduration * 6) && stop == LOW)
  {
    docode();
    code[0] = '\0';
    stop = HIGH;
  }
  
  /*
  if(filteredstate == HIGH)
  {
    digitalWrite(ledPin, HIGH);
    tone(audioOutPin,target_freq);
  }
  else
  {
    digitalWrite(ledPin, LOW);
    noTone(audioOutPin);
  }
*/
 
  realstatebefore = realstate;
  lasthighduration = highduration;
  filteredstatebefore = filteredstate;
}
#endif  // I2C_H

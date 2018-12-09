/*
 *
 *
 *    SWR.h
 *
 *    SWR calculator.
 *
 *    License: GNU General Public License Version 3.0.
 *    
 *    Copyright (C) 2014-2016 by Matthew K. Roberts, KK5JY. All rights reserved.
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *    
 *    This program is distributed in the hope that it will be useful, but
 *    WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *    
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see: http://www.gnu.org/licenses/
 *
 *
 */

#ifndef SWR_H

// use a voltage calculation (sensor is a voltage sensor, not a power sensor)
#define USE_VOLTAGE_CALC

//
//  SWR calculation type
//
class SWR {
  private:
    byte m_FwdPin, m_RefPin;
    word m_Forward, m_Reflected;
    word m_WindowPWR, m_WindowSWR;
    float m_MaxSWR, m_SWR;
    word m_MinPower;

    // scale factors
    float m_ScaleFWD, m_ScaleREF;

    // the smoothing constant
    float m_AlphaFwd, m_AlphaRef;

  public:
    // get the current alpha
    float AlphaForward() const { return m_AlphaFwd; }

    // set the alpha value
    void AlphaForward(float newValue) {
      if (newValue > 1.0)
        newValue = 1.0;
      if (newValue < 0.01)
        newValue = 0.01;
      m_AlphaFwd = newValue;
    }

    // get the current alpha
    float AlphaReflected() const { return m_AlphaRef; }

    // set the alpha value
    void AlphaReflected(float newValue) {
      if (newValue > 1.0)
        newValue = 1.0;
      if (newValue < 0.01)
        newValue = 0.01;
      m_AlphaRef = newValue;
    }

    // get the maximum SWR value permitted
    float MaxSWR() const { return m_MaxSWR; }
    
    // set the maximum SWR value permitted
    void MaxSWR(float value) {
      if (value >= 2)
        m_MaxSWR = value;
    }
    
    // query raw power A/D readings
    word ForwardRaw() const { return m_Forward; }
    word ReflectedRaw() const { return m_Reflected; }

    // query power levels
    float Forward() const { return m_Forward * m_ScaleFWD; }
    float Reflected() const { return m_Reflected * m_ScaleREF; }

    // get scaling factors
    float ScaleForward() const { return m_ScaleFWD; }
    float ScaleReflected() const { return m_ScaleREF; }

    // set scaling factors
    void ScaleForward(float value) { m_ScaleFWD = value; }
    void ScaleReflected(float value) { m_ScaleREF = value; }
    
    // query the real SWR value
    float Value() const { return m_SWR; }
    
    // query the minimum power value
    word MinPower() const { return m_MinPower; }
    
    // set the minimum power value
    void MinPower(word val) { m_MinPower = val; }

  public:
    //
    //  SWR(...) - constructor
    //
    SWR(byte fwdPin, byte refPin) {
      m_FwdPin = fwdPin;
      m_RefPin = refPin;
      m_MaxSWR = 25.0;
      m_WindowPWR = m_WindowSWR = 0;
      m_MinPower = 0;
      m_ScaleFWD = m_ScaleREF = 1.0;
      m_AlphaFwd = 0.5;
      m_AlphaRef = 0.5;
    }

    //
    //  Poll() - read the SWR from the A/D inputs
    //
    void Poll() {
      // read forward voltage
      m_Forward = (m_AlphaFwd * analogRead(m_FwdPin)) + ((1.0 - m_AlphaFwd) * m_Forward);
      
      // read reverse voltage
      m_Reflected = (m_AlphaRef * analogRead(m_RefPin)) + ((1.0 - m_AlphaRef) * m_Reflected);
      
      // compute SWR and return
      float wf;
      if (m_Reflected == 0 || m_Forward < m_MinPower) {
        wf = 1.0;
      } else if (m_Reflected >= m_Forward) {
        wf = m_MaxSWR;
      } else {
        #ifdef USE_VOLTAGE_CALC
        wf = (float)(m_Forward + m_Reflected) / (float)(m_Forward - m_Reflected);
        #else
        wf = (float)m_Forward / (float)m_Reflected;
        wf = sqrt(wf);
        wf = (1.0 + wf) / (1.0 - wf);
        #endif
        wf = abs(wf);
      }
        
      // clip the SWR at a reasonable value
      if (wf > m_MaxSWR) wf = m_MaxSWR;

      // store the final result
      m_SWR = wf;
    }
};

#endif // SWR_H

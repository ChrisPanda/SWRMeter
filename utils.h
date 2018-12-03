/*
 *
 *
 *    utils.h
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


#ifndef __METERSWR_UTILS_H
#define __METERSWR_UTILS_H

#include <stdlib.h>

//
//  SplitCommand(...) - split a command from its argument
//
inline void SplitCommand(const String &input, String &cmd, String &arg) {
  bool isCmd = true;
  cmd = "";
  arg = "";
  for (int i = 0; i != input.length(); ++i) {
    char ch = input.charAt(i);
    if (ch == '=') {
      isCmd = false;
      continue;
    }
    if (isCmd) {
      cmd += ch;
    } else {
      arg += ch;
    }
  }
}


//
//  FormatFloat(...) - format a floating point number
//
inline void FormatFloat(char *buffer, unsigned len, float value) {
  byte point = 0;
  if (value >= 10) point = 1;
  if (value >= 100) point = 2;
  
  snprintf(buffer, len, "%04lu", (unsigned long)(value * 1000));
  byte off = strnlen(buffer, sizeof(buffer));
  for (byte i = off; i != point; --i) {
    buffer[i + 1] = buffer[i];
  }
  buffer[point + 1] = '.';
}

#endif
// EOF

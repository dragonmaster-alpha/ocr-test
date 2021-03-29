/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   string_utils.h
 * Author: mhill
 *
 * Created on February 20, 2017, 2:14 PM
 */

#ifndef OPENALPR_STRING_UTILS_H
#define OPENALPR_STRING_UTILS_H

#include <iostream>
#include <string.h>
#include <stdint.h>
#include "exports.h"

namespace alprsupport
{

  
  OPENALPRSUPPORT_DLL_EXPORT std::string toString(int value);
  OPENALPRSUPPORT_DLL_EXPORT std::string toString(int64_t value);
  OPENALPRSUPPORT_DLL_EXPORT std::string toString(uint32_t value);
  OPENALPRSUPPORT_DLL_EXPORT std::string toString(float value);
  OPENALPRSUPPORT_DLL_EXPORT std::string toString(double value);

  OPENALPRSUPPORT_DLL_EXPORT std::string replaceAll(std::string str, const std::string& from, const std::string& to);
  
  OPENALPRSUPPORT_DLL_EXPORT std::string &ltrim(std::string &s);
  OPENALPRSUPPORT_DLL_EXPORT std::string &rtrim(std::string &s);
  OPENALPRSUPPORT_DLL_EXPORT std::string &trim(std::string &s);
  
}

#endif /* OPENALPR_STRING_UTILS_H */


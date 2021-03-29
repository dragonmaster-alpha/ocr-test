/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   alprlog.cpp
 * Author: mhill
 * 
 * Created on March 8, 2018, 8:38 AM
 */

#include "alprlog.h"
#include <sstream>
#include <iostream>

using namespace std;

namespace alpr
{

  AlprLog* AlprLog::_instance = 0;

  AlprLog* AlprLog::instance()
  {
      if (AlprLog::_instance == NULL)
          AlprLog::_instance = new AlprLog();
      return AlprLog::_instance;
      
  }

  AlprLog::AlprLog()
  {
  }
  AlprLog::~AlprLog() {
  }

  void AlprLog::setParameters(std::string logger_name, bool write_to_file, 
          std::string filename, size_t max_file_size, int max_num_files) {
  }

  void AlprLog::setLogLevel(AlprLogLevel min_level) {


  }
  void AlprLog::setWriteToString() {
  }

  std::string AlprLog::getLogString() {
    return "";
  }



  void AlprLog::debug(std::string message) {
//    std::cout << message << std::endl; 
  }

  void AlprLog::info(std::string message) {
    std::cout << "INFO: " << message << std::endl;
  }

  void AlprLog::warn(std::string message) {
    std::cout << "WARN: " << message << std::endl;
  }

  void AlprLog::error(std::string message) {
    std::cout << "ERROR: " << message << std::endl;
  }
  

  std::ostringstream& AlprLogEntry::debug()
  {
    this->messageLevel = ALPRDEBUG;
    os << "";
    return os;
  }
  std::ostringstream& AlprLogEntry::info()
  {
    this->messageLevel = ALPRINFO;
    os << "";
    return os;
  }
  std::ostringstream& AlprLogEntry::warn()
  {
    this->messageLevel = ALPRWARNING;
    os << "";
    return os;
  }
  std::ostringstream& AlprLogEntry::error()
  {
    this->messageLevel = ALPRERROR;
    os << "";
    return os;
  }
  AlprLogEntry::~AlprLogEntry()
  {

  }

  AlprLogEntry& AlprLogEntry::operator=(const AlprLogEntry& other) {
    this->messageLevel = other.messageLevel;
    os << other.os.rdbuf();
    return *this;
  }

  AlprLogEntry::AlprLogEntry(const AlprLogEntry& other) {
    this->messageLevel = other.messageLevel;
    os << other.os.rdbuf();
  }
}

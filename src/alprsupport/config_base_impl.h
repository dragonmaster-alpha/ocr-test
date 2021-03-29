/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   config_base_impl.h
 * Author: mhill
 *
 * Created on March 9, 2018, 7:24 PM
 */

#ifndef CONFIG_BASE_IMPL_H
#define CONFIG_BASE_IMPL_H

#include <iostream>
#include <string>
#include <vector>
#include <stdlib.h>     /* getenv */
#include "simpleini/simpleini.h"

namespace alprsupport
{
  
  class ConfigBaseImpl {
  public:
    
    ConfigBaseImpl(const std::string country, const std::string config_file = "", const std::string runtime_dir = "");
    virtual ~ConfigBaseImpl();

    std::string get_country();
    bool load_country(const std::string country);

    bool loaded;

    std::string getConfigFilePath();


    float       get_float  (std::string key, float default_val);
    int         get_int    (std::string key, int default_val);
    bool        get_boolean(std::string key, bool default_val);
    std::string get_string (std::string key, std::string default_val);

    float       get_float  (std::string country, std::string key, float default_val);
    int         get_int    (std::string country, std::string key, int default_val);
    bool        get_boolean(std::string country, std::string key, bool default_val);
    std::string get_string (std::string country, std::string key, std::string default_val);

    std::string getRuntimeBaseDir();

      
  private:

      std::string active_country;
      
      std::string config_file_path;
      
      CSimpleIniA* ini;
      CSimpleIniA* countryIni;
      CSimpleIniA* defaultIni;
      std::string runtimeBaseDir;

      // Map various country codes to standard ones
      std::map<std::string, std::string> aliases;
      void loadAliases();
      
      void loadCommonValues(std::string configFile);
      void loadCountryValues(std::string configFile, std::string country);
      
  };
  
}
#endif /* CONFIG_BASE_IMPL_H */


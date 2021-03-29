/*
 * Copyright (c) 2015 OpenALPR Technology, Inc.
 * Open source Automated License Plate Recognition [http://www.openalpr.com]
 * 
 * This file is part of OpenALPR.
 * 
 * OpenALPR is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License 
 * version 3 as published by the Free Software Foundation 
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
*/


#ifndef OPENALPR_CONFIGBASE_H
#define OPENALPR_CONFIGBASE_H



#include <stdio.h>
#include "exports.h"
#include <string>
#include <vector>
#include "platform.h"

#define RUNTIME_DIR 		"/runtime_data"
#define KEYPOINTS_DIR		"/keypoints"
#define CASCADE_DIR		"/region/"
#define NEURAL_DETECTOR_DIR     "/detector/"
#define POSTPROCESS_DIR		"/postprocess"

#define CONFIG_FILE 		"/openalpr.conf"

#ifdef _WIN32
#define ROOT_PATH           (alprsupport::getExeDir() + "\\").c_str()
#define DEFAULT_SHARE_DIR   (alprsupport::getExeDir() + "\\..\\usr\\share\\openalpr").c_str()
#define DEFAULT_VAR_DIR     (alprsupport::getExeDir() + "\\..\\var\\lib\\openalpr").c_str()
#define DEFAULT_CONFIG_DIR  (alprsupport::getExeDir() + "\\..\\etc\\openalpr").c_str()
#define DEFAULT_LOG_DIR (alprsupport::getExeDir() + "\\..\\var\\log").c_str()
#else
#define ROOT_PATH           "/"
#define DEFAULT_SHARE_DIR   "/usr/share/openalpr"
#define DEFAULT_VAR_DIR     "/var/lib/openalpr"
#define DEFAULT_CONFIG_DIR  "/etc/openalpr"
#define DEFAULT_LOG_DIR     "/var/log"
#endif

#define DEFAULT_RUNTIME_DATA_DIR 	    (std::string(DEFAULT_SHARE_DIR) + RUNTIME_DIR).c_str()
#define CONFIG_FILE_TEMPLATE_LOCATION 	(std::string(DEFAULT_SHARE_DIR) + "/config/openalpr.defaults.conf").c_str()

#ifndef DEFAULT_CONFIG_FILE
  #define DEFAULT_CONFIG_FILE 	(std::string(DEFAULT_CONFIG_DIR) + CONFIG_FILE).c_str()
#endif

#define ENV_VARIABLE_CONFIG_FILE "OPENALPR_CONFIG_FILE"
#define ENV_VARIABLE_CONFIG_DIR "OPENALPR_CONFIG_DIR"

namespace alprsupport
{

  struct CountryConfig
  {
    std::string country_name;
    std::string country_code;
    std::vector<std::string> aliases;
  };
  
  class ConfigBaseImpl;
  class OPENALPRSUPPORT_DLL_EXPORT ConfigBase
  {

    public:
      ConfigBase(const std::string country, const std::string config_file = "", const std::string runtime_dir = "");
      virtual ~ConfigBase();

      std::string get_country();
      bool load_country(const std::string country);
      
      bool isLoaded();
      
      std::string getConfigFilePath();

      static std::vector<CountryConfig> get_country_list();

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
    
      ConfigBaseImpl* impl;

  };


}
#endif // OPENALPR_CONFIGBASE_H

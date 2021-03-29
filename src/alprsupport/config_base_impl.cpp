/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   config_base_impl.cpp
 * Author: mhill
 * 
 * Created on March 9, 2018, 7:24 PM
 */

#include "config_base_impl.h"
#include "config_base.h"

#include "filesystem.h"
#include "platform.h"
#include "string_utils.h"

#include "simpleini/simpleini.h"

#include "config_helper.h"

using namespace std;

namespace alprsupport
{
  

  ConfigBaseImpl::ConfigBaseImpl(const std::string country, const std::string config_file, const std::string runtime_dir)
  {

    active_country = "";
    string debug_message = "";

    this->loaded = false;

    ini = NULL;
    defaultIni = NULL;
    countryIni = NULL;


    char* envConfigFile;
    envConfigFile = getenv (ENV_VARIABLE_CONFIG_FILE);
    if (config_file.compare("") != 0)
    {
        // User has supplied a config file.  Use that.
      config_file_path = config_file;
      debug_message = "Config file location provided via API";
    }
    else if (envConfigFile != NULL)
    {
      // Environment variable is non-empty.  Use that.
      config_file_path = envConfigFile;
      debug_message = "Config file location provided via environment variable: " + string(ENV_VARIABLE_CONFIG_FILE);
    }
    else if (DirectoryExists(getExeDir().c_str()) && fileExists((getExeDir() + CONFIG_FILE).c_str()))
    {
      config_file_path = getExeDir() + CONFIG_FILE;
      debug_message = "Config file location provided via exe location";
    }
    else
    {
      // Use the default
      config_file_path = DEFAULT_CONFIG_FILE;
      debug_message = "Config file location provided via default location";
    }


    if (fileExists(config_file_path.c_str()) == false && fileExists(CONFIG_FILE_TEMPLATE_LOCATION) == false)
    {
      std::cerr << "--(!) Config file '" << config_file_path << "' does not exist!" << endl;
      std::cerr << "--(!)             You can specify the configuration file location via the command line " << endl;
      std::cerr << "--(!)             or by setting the environment variable '" << ENV_VARIABLE_CONFIG_FILE << "'" << endl;
      return;
    }
    else if (DirectoryExists(config_file_path.c_str()))
    {
      std::cerr << "--(!) Config file '" << config_file_path << "' was specified as a directory, rather than a file!" << endl;
      std::cerr << "--(!)             Please specify the full path to the 'openalpr.conf file'" << endl;
      std::cerr << "--(!)             e.g., /etc/openalpr/openalpr.conf" << endl;
      return;
    }
    
    
    loadCommonValues(config_file_path);

    if (runtime_dir.compare("") != 0)
    {
      // User provided a runtime directory directly into the library.  Use this.
      this->runtimeBaseDir = runtime_dir;
    }
    else if (DirectoryExists(this->runtimeBaseDir.c_str()) == false)
    {
      if (DirectoryExists((getExeDir() + RUNTIME_DIR).c_str()))
      {
        // Runtime dir in the config is invalid and there is a runtime dir in the same dir as the exe.
        this->runtimeBaseDir = getExeDir() + RUNTIME_DIR;
      }
      else if (DirectoryExists(DEFAULT_RUNTIME_DATA_DIR))
      {
        this->runtimeBaseDir = DEFAULT_RUNTIME_DATA_DIR;
      }
    }
            


    if (DirectoryExists(this->runtimeBaseDir.c_str()) == false)
    {
      std::cerr << "--(!) Runtime directory '" << this->runtimeBaseDir << "' does not exist!" << endl;
      std::cerr << "--(!)                   Please update the OpenALPR config file: '" << config_file_path << "'" << endl;
      std::cerr << "--(!)                   to point to the correct location of your runtime_dir" << endl;
      return;
    }

    loadAliases();
    
    bool country_loaded = load_country(country);

    // Handled in the load_country call
    //this->loaded = country_loaded;
  }
  
  ConfigBaseImpl::~ConfigBaseImpl()
  {
    if (ini != NULL)
      delete ini;
    if (defaultIni != NULL)
      delete defaultIni;
    if (countryIni != NULL)
      delete countryIni;
  }

  std::string ConfigBaseImpl::getRuntimeBaseDir() {
    return runtimeBaseDir;
  }

  std::string ConfigBaseImpl::getConfigFilePath() {
    return config_file_path;
  }

  std::string ConfigBaseImpl::get_country() {
    return active_country;
  }

  void ConfigBaseImpl::loadAliases() {
    // When the object first loads, build a list of all alises for the various countries
    
    vector<string> cfgfiles = alprsupport::getFilesInDir((this->runtimeBaseDir + "/config/").c_str());
    for (int i = 0; i < cfgfiles.size(); i++)
    {
      std::string cfgfile = cfgfiles[i];
      if (alprsupport::hasEnding(cfgfile, ".conf"))
      {
        // Open the file and pull all the aliases
        
        CSimpleIniA cfini;
        cfini.LoadFile((this->runtimeBaseDir + "/config/" + cfgfile).c_str());
        std::string raw_aliases = getString(&cfini,"", "aliases", "");
        
        // Parse the comma separated aliases into a list 
        std::istringstream ss(raw_aliases);
        std::string token;

        std::vector<std::string>  parsed_aliases;
        while(std::getline(ss, token, ',')) {
          std::string trimmed_token = trim(token);
          if (trimmed_token.size() > 0)
            parsed_aliases.push_back(trimmed_token);
        }
        
        // The first item on the list is our country, the rest our aliases
        if (parsed_aliases.size() > 0)
        {
          std::string main_country = parsed_aliases[0];
          for (int z = 0; z < parsed_aliases.size(); z++)
          aliases[parsed_aliases[z]] = main_country;
        }
      }
    }
  }

  
  bool ConfigBaseImpl::load_country(const std::string rawcountry) {

    if (aliases.find(rawcountry) == aliases.end())
    {
      std::cerr << "--(!) Invalid country (" << rawcountry << ").  Check " << runtimeBaseDir << "/config/ for valid values." << std::endl;
      this->loaded = false;
      return false;
    }
    std::string actual_country = aliases[rawcountry];
    // Nothing to do, already loaded.
    if (active_country == actual_country)
    {
      this->loaded = true;
      return true;
    }
    
    this->active_country = actual_country;
    
    if (active_country.length() == 0)
    {
      std::cerr << "--(!) Country not specified." << endl;
      this->loaded = false;
      return false;
    }
    
    std::string country_config_file = this->runtimeBaseDir + "/config/" + active_country + ".conf";
    if (fileExists(country_config_file.c_str()) == false)
    {
      std::cerr << "--(!) Country config file '" << country_config_file << "' does not exist.  Missing config for the country: '" << active_country<< "'!" << endl;
      this->loaded = false;
      return false;
    }


    if (countryIni != NULL)
      delete countryIni;
    
    countryIni = new CSimpleIniA();
    countryIni->SetMultiKey(true);
    countryIni->LoadFile(country_config_file.c_str());
    
    this->loaded = true;
    return true;
    
    
  }

  
  void ConfigBaseImpl::loadCommonValues(string configFile)
  {

    if (ini != NULL)
      delete ini;
    if (defaultIni != NULL)
      delete defaultIni;
    
    ini = new CSimpleIniA();
    if (fileExists(configFile.c_str()))
    {
      ini->LoadFile(configFile.c_str());
    }

    
    defaultIni = new CSimpleIniA();
    if (fileExists(CONFIG_FILE_TEMPLATE_LOCATION))
    {
      defaultIni->LoadFile(CONFIG_FILE_TEMPLATE_LOCATION);
    }
    
    runtimeBaseDir = getString(ini,defaultIni, "", "runtime_dir", DEFAULT_RUNTIME_DATA_DIR);
   
    // Special hack to allow config files to work if the package hasn't been installed
    // Cmake will do this replacement on deploy, but this is useful in development
    if (runtimeBaseDir.find("${CMAKE_INSTALL_PREFIX}") >= 0)
      runtimeBaseDir = replaceAll(runtimeBaseDir, "${CMAKE_INSTALL_PREFIX}", "/usr");
    
    std::string detectorString = getString(ini, defaultIni, "", "detector", "lbpcpu");
    std::transform(detectorString.begin(), detectorString.end(), detectorString.begin(), ::tolower);

      

    

  }
  
  bool ConfigBaseImpl::get_boolean(std::string key, bool default_val) {
    return alprsupport::getBoolean(ini, defaultIni, "", key, default_val);
  }

  float ConfigBaseImpl::get_float(std::string key, float default_val) {
    return alprsupport::getFloat(ini, defaultIni, "", key, default_val);
  }

  int ConfigBaseImpl::get_int(std::string key, int default_val) {
    return alprsupport::getInt(ini, defaultIni, "", key, default_val);
  }
  
  std::string ConfigBaseImpl::get_string(std::string key, std::string default_val) {
    return alprsupport::getString(ini, defaultIni, "", key, default_val);
  }
  

  

  bool ConfigBaseImpl::get_boolean(std::string country, std::string key, bool default_val) {
    if (country != active_country)
      load_country(country);
    return alprsupport::getBoolean(countryIni, "", key, default_val);
  }

  float ConfigBaseImpl::get_float(std::string country, std::string key, float default_val) {
    if (country != active_country)
      load_country(country);
    return alprsupport::getFloat(countryIni, "", key, default_val);
  }

  int ConfigBaseImpl::get_int(std::string country, std::string key, int default_val) {
    if (country != active_country)
      load_country(country);
    return alprsupport::getInt(countryIni, "", key, default_val);
  }

  std::string ConfigBaseImpl::get_string(std::string country, std::string key, std::string default_val) {
    if (country != active_country)
      load_country(country);
    return alprsupport::getString(countryIni, "", key, default_val);
  }
  


}

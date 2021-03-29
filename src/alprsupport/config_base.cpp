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

#include "config_base.h"
#include "config_base_impl.h"
#include "filesystem.h"
#include "platform.h"
#include "string_utils.h"

#include "simpleini/simpleini.h"

#include "config_helper.h"

using namespace std;

namespace alprsupport {

  ConfigBase::ConfigBase(const std::string country, const std::string config_file, const std::string runtime_dir)
  {
    impl = new ConfigBaseImpl(country, config_file, runtime_dir);
  }
  
  ConfigBase::~ConfigBase()
  {
    delete impl;
  }
  
  float ConfigBase::get_float(std::string key, float default_val)
  {
    return impl->get_float(key, default_val);
  }

  std::string ConfigBase::getConfigFilePath() {
    return impl->getConfigFilePath();
  }

  std::string ConfigBase::getRuntimeBaseDir() {
    return impl->getRuntimeBaseDir();
  }

  bool ConfigBase::get_boolean(std::string key, bool default_val) {
    return impl->get_boolean( key, default_val);
  }

  bool ConfigBase::get_boolean(std::string country, std::string key, bool default_val) {
    return impl->get_boolean(country, key, default_val);
  }

  std::string ConfigBase::get_country() {
    return impl->get_country();
  }


  float ConfigBase::get_float(std::string country, std::string key, float default_val) {
    return impl->get_int( country, key, default_val);
  }

  int ConfigBase::get_int(std::string key, int default_val) {
    return impl->get_int( key, default_val);
  }

  int ConfigBase::get_int(std::string country, std::string key, int default_val) {
    return impl->get_int( country, key, default_val);
  }

  std::string ConfigBase::get_string(std::string key, std::string default_val) {
    return impl->get_string( key, default_val);
  }

  std::string ConfigBase::get_string(std::string country, std::string key, std::string default_val) {
    return impl->get_string(country, key, default_val);
  }

  bool ConfigBase::load_country(const std::string country) {
    return impl->load_country(country);
  }

  bool ConfigBase::isLoaded() {
    return impl->loaded;
  }


  std::vector<CountryConfig> ConfigBase::get_country_list() {

    // Load all the ini files in runtime_data/config and grab the country name

    std::vector<CountryConfig> response;

    std::string country_config_dir = std::string(DEFAULT_RUNTIME_DATA_DIR) + "/config/";

    std::vector<std::string> all_files = getFilesInDir(country_config_dir.c_str());

    for (int i = 0; i < all_files.size(); i++)
    {
      if (hasEndingInsensitive(all_files[i], ".conf"))
      {
        std::stringstream full_path;
        full_path << country_config_dir << all_files[i];
        CSimpleIniA ini;
        ini.LoadFile(full_path.str().c_str());

        // Aliases
        std::string aliases = getString(&ini, "", "aliases", "");

        CountryConfig cc;
        cc.country_name = getString(&ini, "", "name", "");

        std::stringstream aliasss( aliases );

        while( aliasss.good() )
        {
            std::string substr;
            getline( aliasss, substr, ',' );
            cc.aliases.push_back( substr );
        }

        cc.country_code = cc.aliases[0];

        response.push_back(cc);
      }
    }

    return response;
  }

}



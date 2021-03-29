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

#include "config.h"
#include <iostream>
using std::cout;
using std::endl;

namespace alpr {
Config::Config(const string country, const string config_file, const string runtime_dir)
              : base(country, config_file, runtime_dir) {
  tracing_enabled = false;
  if (const char* tracing_val = std::getenv("OPENALPR_TRACING")) {
    if (strcmp(tracing_val, "1") == 0)
      tracing_enabled = true;
  }

  skip_tensorrt = false;
  if (const char* skiptrt_val = std::getenv("OPENALPR_SKIP_TENSORRT")) {
    if (strcmp(skiptrt_val, "1") == 0)
      skip_tensorrt = true;
  }

  maxPlateWidthPercent = base.get_float("max_plate_width_percent", 100);
  maxPlateHeightPercent = base.get_float("max_plate_height_percent", 100);
  maxDetectionSize.width = base.get_int("max_detection_input_width", 1280);
  maxDetectionSize.height = base.get_int("max_detection_input_height", 768);
  // input sizes for edge neural nets.
  plateDetectorSize.width = base.get_int("plate_detection_input_width", 960);
  plateDetectorSize.height = base.get_int("plate_detection_input_height", 540);
  // I'm thinking these won't be used now.. since we get the x/y coordinates based from mask.
  plateDetectorPt0.x = base.get_int("plate_detection_pt0_x", -1);
  plateDetectorPt0.y = base.get_int("plate_detection_pt0_y", -1);
  plateDetectorPt1.x = base.get_int("plate_detection_pt1_x", -1);
  plateDetectorPt1.y = base.get_int("plate_detection_pt1_y", -1);

  vehicleDetectorSize.width = base.get_int("vehicle_detection_input_width", 300);
  vehicleDetectorSize.height = base.get_int("vehicle_detection_input_height", 300);

  // how many Tensorrt instances to use. If this is zero we are going to assume that the intention is to use the
  // default strategy for gpu.
  num_trt_gpu_instances = base.get_int("num_trt_gpu_instances", 0);

  // various
  mustMatchPattern = base.get_boolean("must_match_pattern", false);
  skipDetection = base.get_boolean("skip_detection", false);
  skipRecognition = base.get_boolean("skip_recognition", false);
  skipVehicleDetection = base.get_boolean("skip_vehicle_detection", false);
  detection_mask_image = base.get_string("detection_mask_image", "");
  prewarp = base.get_string("prewarp", "");

  hardware_acceleration = ALPRCONFIG_CPU;
  gpu_id = 0;
  gpu_batch_size = 1;

  postProcessMinConfidence = base.get_float("postprocess_min_confidence", 100);
  postProcessConfidenceSkipLevel = base.get_float("postprocess_confidence_skip_level", 100);

  debugGeneral = base.get_boolean("debug_general", false);
  debugTiming = base.get_boolean("debug_timing", false);
  debugPrewarp = base.get_boolean("debug_prewarp", false);
  debugDetector = base.get_boolean("debug_detector", false);
  debugOcr = base.get_boolean("debug_ocr", false);
  debugPostProcess = base.get_boolean("debug_postprocess", false);
  debugShowImages = base.get_boolean("debug_show_images", false);
  debugPauseOnFrame = base.get_boolean("debug_pause_on_frame", false);

  setCountry(country);

  ocr_runtime_dir = this->base.getRuntimeBaseDir() + "/ocr/";
  post_process_runtime_dir = this->base.getRuntimeBaseDir() + POSTPROCESS_DIR;
  config_file_path = base.getConfigFilePath();
  runtime_base_dir = base.getRuntimeBaseDir();
}

Config::~Config() {
}

bool Config::loaded() {
  return base.isLoaded();
}

void Config::setCountry(string country) {
  base.load_country(country);

  minPlateSize.width = base.get_int(country, "min_plate_size_width_px", 100);
  minPlateSize.height = base.get_int(country, "min_plate_size_height_px", 100);

  plateWidthMM = base.get_float(country, "plate_width_mm", 100);
  plateHeightMM = base.get_float(country, "plate_height_mm", 100);

  detectorLanguage = base.get_string(country, "detector_language", "");

  ocrLanguage = base.get_string(country, "ocr_language", "none");

  vehicleLanguage = base.get_string(country, "vehicle_language", "us");


  postProcessRegexLetters = base.get_string(country, "postprocess_regex_letters", "\\pL");
  postProcessRegexNumbers = base.get_string(country,  "postprocess_regex_numbers", "\\pN");

  ocrSize.width = base.get_int(country, "ocr_crop_width_px", 100);
  ocrSize.height = base.get_int(country,  "ocr_crop_height_px", 100);

  postProcessMinCharacters = base.get_int(country, "postprocess_min_characters", 4);
  postProcessMaxCharacters = base.get_int(country, "postprocess_max_characters", 8);
}


void Config::setDebug(bool value) {
  debugGeneral = value;
  debugTiming = value;
  debugPrewarp = value;
  debugDetector = value;
  debugOcr = value;
  debugPostProcess = value;
  debugShowImages = value;
}

string Config::getPostProcessRuntimeDir() const {
  return post_process_runtime_dir;
}

string Config::getOCRPrefix() const {
  return ocr_runtime_dir;
}

string Config::getConfigFilePath() const {
  return config_file_path;
}

string Config::getRuntimeBaseDir() const {
  return runtime_base_dir;
}

}  // namespace alpr

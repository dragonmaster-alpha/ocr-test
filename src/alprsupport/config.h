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

#ifndef ALPR_SRC_OPENALPR_CONFIG_H_
#define ALPR_SRC_OPENALPR_CONFIG_H_

#include <alprsupport/config_base.h>
#include <string>
using std::string;

namespace alpr {
enum AlprAccelerationDevice {
  ALPRCONFIG_CPU = 0,
  ALPRCONFIG_NVIDIA_GPU = 1
};

typedef struct dims_t {
  int width;
  int height;
} dims_t;

typedef struct point_2d_t {
  int x;
  int y;
} point_2d_t;

class OPENALPRSUPPORT_DLL_EXPORT Config {
 public:
    explicit Config(const string country, const string config_file = "", const string runtime_dir = "");
    virtual ~Config();

    string getCountry() { return base.get_country(); }
    void setCountry(string country);

    void setDebug(bool value);

    bool loaded();



    float maxPlateWidthPercent;
    float maxPlateHeightPercent;
    dims_t maxDetectionSize;
    dims_t vehicleDetectorSize;
    /*
      the way I'm thinking about the following 3 (6 really) options is:
      1) if plateDetectorPt0/1 aren't set (== -1), then take the full frame and scale it to plateDetectorSize
      2) if plateDetectorPt0/1 *are* set, then crop that area and resize to plateDetectorSize.
      User flow should be something like this:
       select plateDetectorPt0/1 -> define plateDetectorSize -> Apply any mask to that area.
    */
    dims_t plateDetectorSize;
    point_2d_t plateDetectorPt0;
    point_2d_t plateDetectorPt1;

    bool skipDetection;
    bool skipRecognition;
    bool skipVehicleDetection;
    string detection_mask_image;

    string prewarp;

    dims_t minPlateSize;

    float plateWidthMM;
    float plateHeightMM;

    int crop_jpeg_quality;
    
    AlprAccelerationDevice hardware_acceleration;
    int gpu_id;
    int gpu_batch_size;

    dims_t ocrSize;

    string detectorLanguage;

    string ocrLanguage;

    string vehicleLanguage;

    bool mustMatchPattern;

    float postProcessMinConfidence;
    float postProcessConfidenceSkipLevel;
    unsigned int postProcessMinCharacters;
    unsigned int postProcessMaxCharacters;

    string postProcessRegexLetters;
    string postProcessRegexNumbers;

    bool debugGeneral;
    bool debugTiming;
    bool debugPrewarp;
    bool debugDetector;
    bool debugOcr;
    bool debugPostProcess;
    bool debugShowImages;
    bool debugPauseOnFrame;

    bool tracing_enabled;
    bool skip_tensorrt;

    string getPostProcessRuntimeDir() const;
    string getOCRPrefix() const;
    string getConfigFilePath() const;
    string getRuntimeBaseDir() const;

    string ocr_runtime_dir;
    string post_process_runtime_dir;
    string config_file_path;
    string runtime_base_dir;

    int num_trt_gpu_instances;

 private:
    alprsupport::ConfigBase base;
};
}  // namespace alpr
#endif  // ALPR_SRC_OPENALPR_CONFIG_H_


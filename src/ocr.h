/*************************************************************************
 * REKOR RECOGNITION SYSTEMS CONFIDENTIAL
 *
 *  Copyright 2020 Rekor Recognition Systems, Inc.
 *  All Rights Reserved.
 *
 * NOTICE:  All information contained herein is, and remains
 * the property of Rekor Recognition Systems Incorporated. The intellectual
 * and technical concepts contained herein are proprietary to Rekor Recognition
 * Systems Incorporated and may be covered by U.S. and Foreign Patents.
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from Rekor Recognition Systems Technology Incorporated.
 */

#ifndef OPENALPR_OCR_OCR_H_
#define OPENALPR_OCR_OCR_H_
#include "config.h"
#include "postprocess/postprocess.h"
#include <onnxruntime/core/session/onnxruntime_c_api.h>
#include "alprlog/alprlog.h"
#include <unordered_map>
#ifdef _WIN32
  #define OCR_DLL_EXPORT __declspec(dllexport)
#else
  #define OCR_DLL_EXPORT
#endif


namespace alpr {
struct OcrChar {
  std::string letter;
  int char_index;
  float confidence;
};

struct OcrProvince {
  std::string regioncode;
  float confidence;
};

struct OcrResult {
  int image_index;
  std::vector<cv::Point2f> corner_points;
  std::vector<OcrProvince> provinces;
  std::vector<OcrChar> characters;
  float overall_confidence;
};

struct OcrRequestCrop {
  int image_index;
  int ideal_width;
  int ideal_height;
  std::vector<float> corner_points;
};

class OCR_DLL_EXPORT Ocr {
 public:
  explicit Ocr(Config* config);
  virtual ~Ocr();
  bool initialized() { return _initialized; }
  std::vector<OcrResult> recognize_batch(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops);
  void initialize_input_tensor(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops);

 private:
  std::vector<OcrResult> recognize_sub_batch(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops);
  bool append_character(OcrResult& word, int char_index, std::vector<std::pair<int, float>>& sorted_softmax);


  Config* config;
  bool _initialized;
  int max_batch;
  int batch_clamp_size;
  int crop_width;
  int crop_height;
  const int crop_channels = 3;
  int topk;
  int max_timesteps;
  std::unordered_map<int, std::string> char_name_map;
  std::unordered_map<int, std::string> region_name_map;
  bool has_regions;
  OrtEnv* env;
  OrtSession* session;
  OrtSessionOptions* session_options;
  // Reusable memory used to house the image data
  float* input_tensor_values;
  size_t input_tensor_max_size;
  size_t total_crops_processed;
  size_t input_tensor_size;
};
}  // namespace alpr
#endif  // OPENALPR_OCR_OCR_H_

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

#include "ocr.h"
#include "backend/onnxruntime.h"
#include "opencv2/imgproc/imgproc.hpp"
#include <alprsupport/json.hpp>
#include <alprsupport/filesystem.h>
#include <alprsupport/profiler.h>
#include <alprlog.h>
#include <alprgpusupport.h>
#include <vector>

using namespace std;
using namespace cv;

#define MIN_OCR_OVERALL_CONFIDENCE 0.1f
#define MAX_OCR_OVERALL_CONFIDENCE_MULTIPLIER 0.95f
#define MIN_OCR_CONFIDENCE_ADJUSTMENT 0.6f

namespace alpr {
Ocr::Ocr(Config* config) {
  this->config = config;
  _initialized = false;
  has_regions = false;
  this->total_crops_processed = 0;
  set_omp_to_synchronous();
  input_tensor_values = NULL;
  input_tensor_max_size = 0;

  std::string ocr_root = "/home/mhill/Downloads/ocr_test/runtime";

  // std::string ocr_root = config->getOCRPrefix() + config->ocrLanguage;
  std::string ocr_model_path = ocr_root + "/ocr_x";
  if (!alprsupport::fileExists(ocr_model_path.c_str()))
    ocr_model_path = ocr_root + "/ocr_x.enc";

  if (!alprsupport::fileExists(ocr_model_path.c_str())) {
    ALPR_ERROR << "Unable to find OCR runtime data: " << ocr_model_path;
    return;
  }

  std::string ocr_config_path = ocr_root + "/ocr_config.json";
  if (!alprsupport::fileExists(ocr_config_path.c_str())) {
    ALPR_ERROR << "Unable to find OCR configuration: " << ocr_config_path;
    return;
  }

  // Read runtime constants from JSON
  std::ifstream ifs(ocr_config_path);
  nlohmann::json runtime = nlohmann::json::parse(ifs);

  // Define input/output nodes and shapes
  topk = runtime["topk"];
  crop_width = runtime["crop_width"];
  crop_height = runtime["crop_height"];
  max_timesteps = runtime["max_timesteps"];

  // Fill the char toke and region token maps from the JSON values
  for (auto & x : runtime["region_idx2name"].items())
    region_name_map[stoi(x.key())] = x.value();

  for (auto & x : runtime["idx2char"].items())
    char_name_map[stoi(x.key())] = x.value();

  has_regions = region_name_map.size() > 0;
  input_tensor_size = crop_height * crop_width * crop_channels * sizeof(float);
  // Initialize session options if needed
  OrtCheckStatus(g_ort->CreateSessionOptions(&session_options));
  OrtCheckStatus(g_ort->SetIntraOpNumThreads(session_options, 1));
  OrtCheckStatus(g_ort->SetSessionGraphOptimizationLevel(session_options, ORT_ENABLE_ALL));

  // g_ort->EnableProfiling(session_options, "onnxprof");
  // Create session and load model into memory
  if (config->hardware_acceleration == ALPRCONFIG_NVIDIA_GPU) {
    // GPU
    const bool prefer_tensorrt = false;
    AlprGpuSupport* alpr_gpu_support = AlprGpuSupport::getInstance(config->gpu_id);
    alpr_gpu_support->set_onnxruntime_gpu(session_options, prefer_tensorrt);

    const int OCR_BATCH_MULTIPLE = 4;
    this->max_batch = config->gpu_batch_size * OCR_BATCH_MULTIPLE;
    this->batch_clamp_size = min(10, config->gpu_batch_size / 2);
  } else {
    // CPU
    this->max_batch = 100;
  }
  std::vector<char> filedata = read_model(ocr_model_path.c_str(), "ocr");
  OrtCheckStatus(g_ort->CreateSessionFromArray(g_env, filedata.data(), filedata.size(), session_options, &session));
  _initialized = true;
}


Ocr::~Ocr() {
  g_ort->ReleaseSession(session);
  g_ort->ReleaseSessionOptions(session_options);
  if (input_tensor_values != NULL && config->hardware_acceleration == ALPRCONFIG_CPU)
    free(input_tensor_values);
}


void Ocr::initialize_input_tensor(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops) {
  // assuming network expects RGB, and images is in BGR.
  const int NUM_CHANNELS = 3;
  const int channel_order[] = {2, 1, 0};
  const int channel_stride = crop_width * crop_height;
  if (images.size() == 0 || crops.size() == 0)
    return;

  ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "Initialize Input Crops");
  size_t total_bytes = crops.size() * input_tensor_size;
  if (total_bytes > input_tensor_max_size) {
    if (input_tensor_values != NULL)
      free(input_tensor_values);
    input_tensor_max_size = total_bytes;
    input_tensor_values = static_cast<float *>(malloc(input_tensor_max_size));
  }

  Mat scratchspace(crop_height, crop_width, CV_8UC3);
  Size cropSize = Size(crop_width, crop_height);

  std::vector<Point2f> small_corners;
  small_corners.push_back(Point2f(0, 0));
  small_corners.push_back(Point2f(crop_width, 0));
  small_corners.push_back(Point2f(crop_width, crop_height));
  small_corners.push_back(Point2f(0, crop_height));

  float * tmp = input_tensor_values;
  for (uint32_t i = 0; i < crops.size(); i++) {
    cv::Mat original_image = images[crops[i].image_index];
    std::vector<Point2f> big_corners;
    for (int z = 0; z < crops[i].corner_points.size(); z = z+2)
      big_corners.push_back(Point2f(crops[i].corner_points[z], crops[i].corner_points[z+1]));

    Mat transmtx = getPerspectiveTransform(big_corners, small_corners);
    Mat crop_image(crop_height, crop_width, original_image.type());
    warpPerspective(original_image, crop_image, transmtx, cropSize, INTER_LINEAR, BORDER_REPLICATE, Scalar());

    // Load image data to ORT format (channel blocks by row): https://answers.opencv.org/question/64837
    // Mat crop_image_rgb;
    // if (crop_image.channels() == 1)
    //   cv::cvtColor(crop_image, crop_image_rgb, CV_GRAY2RGB);
    // else
    //   cv::cvtColor(crop_image, crop_image_rgb, cv::COLOR_BGR2RGB);
    Mat float_image;
    crop_image.convertTo(float_image, CV_32F);
    std::vector<cv::Mat> channels;
    channels.clear();
    for (int i = 0; i < NUM_CHANNELS; ++i) {
      float * tmp_buffer = tmp + channel_order[i] * channel_stride;
      cv::Mat channel(crop_height, crop_width, CV_32FC1, tmp_buffer);
      channels.push_back(channel);
    }
    tmp += NUM_CHANNELS * channel_stride;
    cv::split(float_image, channels);
    // cv::imwrite("/tmp/test.png", crop_image);
    // cv::imshow("test", crop_image);
    // cv::waitKey(0);
  }
  ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
}


// Return true if it should keep going.  False if it hits a stop character
bool Ocr::append_character(OcrResult& word, int char_index, std::vector<std::pair<int, float>>& sorted_softmax) {
  const float MIN_CHAR_CONFIDENCE = 0.0005;
  for (uint32_t i = 0; i < sorted_softmax.size(); i++) {
    if (sorted_softmax[i].second < MIN_CHAR_CONFIDENCE)
      continue;

    OcrChar c;
    c.char_index = char_index;
    c.confidence = sorted_softmax[i].second;
    c.letter = char_name_map[sorted_softmax[i].first];

    if (i == 0) {
      // * = padding or start of seq -- shouldn't ever show up in the output
      if (c.letter == "*" || c.letter == "^") {
        ALPR_DEBUG << "Seeing an unexpected " << c.letter << " output in OCR";
        continue;
      }
      if (word.overall_confidence < 0)
        word.overall_confidence = c.confidence;
      else
        word.overall_confidence *= c.confidence;

      if (c.letter == "~")  // ~ = negative plate
        return false;
      if (c.letter == "$")  // ~ End of sequence
        return false;
    }
    if (c.letter != "$")
      word.characters.push_back(c);
  }
  return true;
}

std::vector<OcrResult> Ocr::recognize_batch(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops) {
  std::vector<OcrResult> results;
  auto profiler = alprsupport::Profiler::Get();
  if (profiler->isON()) {
    ALPR_PROF_SCOPE_START(profiler, string("OCR Batch (" + std::to_string(crops.size())+")").c_str());
  }

  // Only send up to the max_batch at a time
  for (uint32_t i = 0; i < crops.size(); i = i + max_batch) {
    int start_index = i;
    int end_index = i + max_batch;
    if (end_index >= crops.size())
      end_index = crops.size();
    std::vector<OcrRequestCrop> crop_slice = std::vector<OcrRequestCrop>(crops.begin() + start_index,
                                                                         crops.begin() + end_index);
    ALPR_PROF_SCOPE_START(profiler, "recognize_sub_batch");
    std::vector<OcrResult> subresults = recognize_sub_batch(images, crop_slice);
    for (int j = 0; j < subresults.size(); j++) {
      OcrResult r = subresults[j];
      results.push_back(r);
    }
    ALPR_PROF_SCOPE_END(profiler);
  }
  ALPR_PROF_SCOPE_END(profiler);
  return results;
}

std::vector<OcrResult> Ocr::recognize_sub_batch(std::vector<cv::Mat>& images, std::vector<OcrRequestCrop> crops) {
    timespec startTime;
    alprsupport::getTimeMonotonic(&startTime);
    std::vector<OcrResult> response;
    if (crops.size() == 0)
      return response;

    int num_output_names;
    std::vector<const char*> output_node_names;
    if (has_regions) {
      output_node_names = {"character_ids", "region_ids", "character_confidences", "region_confidences"};
    } else {
      output_node_names = {"character_ids", "character_confidences"};
    }
    num_output_names = output_node_names.size();
    const char* input_node_names[] = {"images"};
    size_t input_memory_size;
    int batch_size = crops.size();
    if (config->hardware_acceleration == ALPRCONFIG_NVIDIA_GPU) {
      AlprGpuSupport* alpr_gpu_support = AlprGpuSupport::getInstance(config->gpu_id);
      std::vector<OcrCropInfo> crop_info;
      for (int i = 0; i < crops.size(); i++) {
        OcrCropInfo c;
        c.image_index = crops[i].image_index;
        c.x1 = crops[i].corner_points[0];
        c.y1 = crops[i].corner_points[1];
        c.x2 = crops[i].corner_points[2];
        c.y2 = crops[i].corner_points[3];
        c.x3 = crops[i].corner_points[4];
        c.y3 = crops[i].corner_points[5];
        c.x4 = crops[i].corner_points[6];
        c.y4 = crops[i].corner_points[7];
        crop_info.push_back(c);
      }
      input_tensor_values = (float*) alpr_gpu_support->get_gpu_ocr_crops(max_batch,
                                          crops[0].ideal_width, crops[0].ideal_height, crop_info, input_memory_size);

      // GPU caching for the CUDA provider is goofy.  For every different batch size, it goes through a caching
      // operation which is slow. For example, if we have a max 40 image batch, and pass 6 images in, the first
      // time we do this it will take 300ms, the second time ~5ms
      // Then this will happen again if we send in 7 images.
      // As a mitigation, we'll clamp the batch size that is sent to a slightly larger number
      // We'll increase the memory size to the next one up (e.g., 4 plates would send a batch size of
      // 5, 6 plates would send 10, etc)
      batch_size = ((crops.size() / this->batch_clamp_size) + 1) * this->batch_clamp_size;
      if (batch_size > max_batch)
        batch_size = max_batch;
      input_memory_size = crop_width * crop_height * crop_channels * batch_size * sizeof(float);
    } else {
      initialize_input_tensor(images, crops);
      input_memory_size = crops.size() * input_tensor_size;
    }

    // Create input tensor object from data values
    ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "Create input");
    OrtMemoryInfo* memory_info;
    OrtCheckStatus(g_ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &memory_info));
    OrtValue* input_tensor = NULL;

    std::vector<int64_t> input_shape = {batch_size, crop_channels, crop_height, crop_width};
    size_t input_shape_len = 4;
    OrtCheckStatus(g_ort->CreateTensorWithDataAsOrtValue(memory_info, input_tensor_values, input_memory_size,
                                                         input_shape.data(), input_shape.size(),
                                                         ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor));
    ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
    int is_tensor;
    OrtCheckStatus(g_ort->IsTensor(input_tensor, &is_tensor));
    assert(is_tensor);
    g_ort->ReleaseMemoryInfo(memory_info);

    // Run inference
    ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "inference");
    std::vector<OrtValue*> outputs(num_output_names, nullptr);
    OrtCheckStatus(g_ort->Run(session, NULL, input_node_names, &input_tensor, 1, output_node_names.data(),
                               num_output_names, outputs.data()));
    OrtCheckStatus(g_ort->IsTensor(outputs[0], &is_tensor));
    assert(is_tensor);
    ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());

    // Get pointers to output tensor float values
    int64_t* char_ids;
    int64_t* region_ids;
    float* char_confids;
    float* region_confids;
    if (has_regions) {
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[0], (void**)&char_ids));
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[1], (void**)&region_ids));
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[2], (void**)&char_confids));
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[3], (void**)&region_confids));
    } else {
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[0], (void**)&char_ids));
      OrtCheckStatus(g_ort->GetTensorMutableData(outputs[1], (void**)&char_confids));
    }


    // Determine highest confidence region and character classes
    ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "parse float output");
    std::vector<std::pair<int, float>> top_tokens(topk);

    for (int item_idx = 0; item_idx < crops.size(); item_idx++) {
      OcrResult result;
      if (has_regions) {
        int topk_regions = std::min<int>(topk, region_name_map.size());  // Some models may have < 10 regions
        for (uint32_t k = 0; k < topk_regions; k++) {
          int idx = item_idx*topk_regions + k;
          top_tokens[k] = std::make_pair(static_cast<int>(region_ids[idx] & 0xFFFFFFFF), region_confids[idx]);
        }
        for (auto region : top_tokens) {
          OcrProvince p;
          p.regioncode = region_name_map[region.first];
          p.confidence = region.second * 100;
          result.provinces.push_back(p);
        }
      }

      result.overall_confidence = -1;
      for (int z = 0; z < crops[item_idx].corner_points.size(); z = z+2)
        result.corner_points.push_back(Point2f(crops[item_idx].corner_points[z], crops[item_idx].corner_points[z+1]));

      result.image_index = crops[item_idx].image_index;

      for (int t = 0; t < max_timesteps; t++) {
        for (uint32_t k = 0; k < topk; k++) {
          int idx = item_idx * topk * max_timesteps + t * topk + k;
          top_tokens[k] = std::make_pair(static_cast<int>(char_ids[idx] & 0xFFFFFFFF), char_confids[idx]);
        }
        bool keep_going = append_character(result, t, top_tokens);
        if (!keep_going)
          break;
      }
      // Apply some filters here on the final output
      float confidence_multiplier = ((1.0 - MIN_OCR_CONFIDENCE_ADJUSTMENT) * result.overall_confidence) + MIN_OCR_CONFIDENCE_ADJUSTMENT;
      // Don't go above MAX_OCR_OVERALL_CONFIDENCE_MULTIPLIER, ever...
      confidence_multiplier *= MAX_OCR_OVERALL_CONFIDENCE_MULTIPLIER;
      for (uint32_t charidx = 0; charidx < result.characters.size(); charidx++)
        result.characters[charidx].confidence *= confidence_multiplier;

      // If we're below the minimum confidence Skip it.
      if (result.overall_confidence < MIN_OCR_OVERALL_CONFIDENCE) {
        continue;
      }

      result.overall_confidence *= 100;
      for (uint32_t z = 0; z < result.characters.size(); z++)
        result.characters[z].confidence *= 100;
      response.push_back(result);
    }
    ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
    total_crops_processed += crops.size();
    for (uint32_t i = 0; i < num_output_names; i++)
      g_ort->ReleaseValue(outputs[i]);
    g_ort->ReleaseValue(input_tensor);
    return response;
}
}

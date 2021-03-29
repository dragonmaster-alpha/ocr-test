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


#include "alprgpusupport.h"
#include <dlfcn.h>
#include <sstream>
#include <iostream>
#include <alprsupport/profiler.h>
#include <alprlog.h>
#include <stdint.h>
#include <mutex>
#include "stub/alprgpusupport_cudaimpl.h"
using namespace std;



namespace alpr {
  std::mutex mtx;           // mutex for critical section

  static std::map<int, AlprGpuSupport*> instances;

// Static function creates a singleton instance for each GPU
AlprGpuSupport* AlprGpuSupport::getInstance(int gpu_id) {
  mtx.lock();
  if (instances.find(gpu_id) == instances.end()) {
    instances[gpu_id] = new AlprGpuSupport(gpu_id);
    alprgpusupport_initialize_gpu(gpu_id);
  }
  if (instances.count(gpu_id) == 1) {
  mtx.unlock();
    return instances[gpu_id];
  } else {
  mtx.unlock();
    std::cerr << "failed to find gpu id" << std::endl;
    exit(EXIT_FAILURE);
  }
}

AlprGpuSupport::AlprGpuSupport(int gpu_id) {
  this->profiler = static_cast<void *>(alprsupport::Profiler::Get());

  this->_images_preloaded = false;
  this->error_message = "";
  this->last_batch_size = 0;
  this->library_loaded = false;
  this->gpu_id = gpu_id;
  if (gpu_id < 0) {
    library_loaded = false;
    ALPR_ERROR << "gpu_id can't be negative";
    return;
  }

  const int ERROR_BUFFER_LENGTH = 250;
  char error_buffer[ERROR_BUFFER_LENGTH];
  bool has_gpu = alprgpusupport_has_gpu_support(gpu_id, error_buffer, ERROR_BUFFER_LENGTH);
  if (!has_gpu) {
    error_message = error_buffer;
    library_loaded = false;
    ALPR_ERROR << error_message;
    return;
  }
  library_loaded = true;
}

bool AlprGpuSupport::is_jetson() {
  return alprgpusupport_is_jetson();
}

AlprGpuSupport::~AlprGpuSupport() {
}

void AlprGpuSupport::destroy() {
  alprgpusupport_destroy(gpu_id);
  if (instances.find(gpu_id) != instances.end()) {
    instances.erase(gpu_id);
  }
}


void AlprGpuSupport::upload_batch(vector<unsigned char*> mat_vectors, int bytes_per_pixel, int width, int height) {
  if (!library_loaded)
    return;
  this->last_batch_size = mat_vectors.size();
  ALPR_PROF_SCOPE_START(((alprsupport::Profiler*) profiler), "Upload GPU Image Batch");
  std::vector<unsigned char*> data;
  for (int i = 0; i < mat_vectors.size(); i++) {
    data.push_back(mat_vectors[i]);
  }
  alprgpusupport_upload_batch(gpu_id, data.data(), bytes_per_pixel, width, height, mat_vectors.size());
  ALPR_PROF_SCOPE_END(((alprsupport::Profiler*) profiler));
}


void AlprGpuSupport::ResizeBatch(float * output, int batch, int width, int height, int buf_width, int buf_height,
                                 cv::Rect mask) {
  ALPR_PROF_SCOPE_START(((alprsupport::Profiler*) profiler), "GPU Resize Batch");
  std::vector<int> m_mask = {mask.x, mask.y, mask.width, mask.height};
  alprgpusupport_resize_batch(gpu_id, batch, output, width, height, buf_width, buf_height, m_mask.data());
  ALPR_PROF_SCOPE_END(((alprsupport::Profiler*) profiler));
}


void AlprGpuSupport::set_max_batch_size(int max_batch_size) {
  alprgpusupport_set_max_batch_size(gpu_id, max_batch_size);
}

char* AlprGpuSupport::encode_jpeg(int image_index, void* data_pointer, size_t& jpeg_size, int jpeg_quality) {
  const int ERROR_BUFFER_LENGTH = 250;
  char error_buffer[ERROR_BUFFER_LENGTH];
  if (!alprgpusupport_has_gpu_support(gpu_id, error_buffer, ERROR_BUFFER_LENGTH)) {
    error_message = "GPU is not available";
    return NULL;
  }

  ALPR_PROF_SCOPE_START(((alprsupport::Profiler*) profiler), "Encode GPU JPEG");
  int bytecount = 0;
  char* response = alprgpusupport_encode_jpeg(gpu_id, image_index, &bytecount, jpeg_quality, data_pointer);
  jpeg_size = bytecount;
  ALPR_PROF_SCOPE_END(((alprsupport::Profiler*) profiler));
  return response;
}

char* AlprGpuSupport::encode_jpeg(int image_index, size_t& jpeg_size, int jpeg_quality) {
  return encode_jpeg(image_index, NULL, jpeg_size, jpeg_quality);
}
char* AlprGpuSupport::encode_jpeg(void* gpu_img_pointer, int pitch, int img_width, int img_height,
                                  void* out_data_pointer, size_t& jpeg_size, int jpeg_quality) {
  const int ERROR_BUFFER_LENGTH = 250;
  char error_buffer[ERROR_BUFFER_LENGTH];
  if (!alprgpusupport_has_gpu_support(gpu_id, error_buffer, ERROR_BUFFER_LENGTH)) {
    error_message = "GPU is not available";
    return NULL;
  }

  ALPR_PROF_SCOPE_START(((alprsupport::Profiler*) profiler), "Encode GPU JPEG");
  int bytecount = 0;

  char* response = alprgpusupport_encode_jpeg_from_mat(gpu_id, gpu_img_pointer, pitch, img_width, img_height,
                                                       &bytecount, jpeg_quality, out_data_pointer);
  jpeg_size = bytecount;
  ALPR_PROF_SCOPE_END(((alprsupport::Profiler*) profiler));
  return response;
}

char* AlprGpuSupport::encode_jpeg_crop(int image_index, cv::Rect crop_region, cv::Size output_size,
                                       size_t& jpeg_size, int jpeg_quality) {
  int bytecount = 0;
  ALPR_PROF_SCOPE_START(((alprsupport::Profiler*) profiler), "Encode GPU JPEG Crop");

  char* data = alprgpusupport_encode_jpeg_crop(gpu_id, image_index, &bytecount, jpeg_quality, crop_region.x,
                                               crop_region.y, crop_region.width, crop_region.height, output_size.width,
                                               output_size.height);
  jpeg_size = bytecount;
  ALPR_PROF_SCOPE_END(((alprsupport::Profiler*) profiler));
  return data;
}


void AlprGpuSupport::release_jpeg(char *encoded_jpeg) {
  alprgpusupport_release_jpeg(encoded_jpeg);
}

GpuMatInfo AlprGpuSupport::get_gpu_image_pointer_bgr(int image_index) {
  GpuMatInfo info;
  alprgpusupport_get_bgr_gpu_mat(gpu_id, image_index, &info.device_pointer, &info.width, &info.height, &info.type,
                                 &info.step);
  if (info.device_pointer == NULL)
    info.mat_found = false;
  else
    info.mat_found = true;
  return info;
}

GpuMatInfo AlprGpuSupport::get_gpu_image_pointer_channel(int image_index, int channel_index) {
  GpuMatInfo info;
  void * data;
  alprgpusupport_get_singlechannel_gpu_mat(gpu_id, image_index, channel_index, &data, &info.width,
                                           &info.height, &info.type, &info.step);
  info.device_pointer = static_cast<int *>(data);
  if (info.device_pointer == NULL)
    info.mat_found = false;
  else
    info.mat_found = true;
  return info;
}

float* AlprGpuSupport::get_gpu_ocr_crops(int max_batch_size, int crop_width, int crop_height,
                                         std::vector<OcrCropInfo> crop_info, size_t& data_size) {
  std::vector<float> corners;
  float* data_pointer = NULL;

  for (OcrCropInfo& c : crop_info) {
    corners.push_back(static_cast<float>(c.image_index));
    corners.push_back(c.x1);
    corners.push_back(c.y1);
    corners.push_back(c.x2);
    corners.push_back(c.y2);
    corners.push_back(c.x3);
    corners.push_back(c.y3);
    corners.push_back(c.x4);
    corners.push_back(c.y4);
  }

  alprgpusupport_get_ocr_crops(gpu_id, max_batch_size, crop_width, crop_height, crop_info.size(), corners.data(),
                               &data_pointer, &data_size);
  return data_pointer;
}

void AlprGpuSupport::decode_jpeg_and_crop(const char* jpeg_data, size_t jpeg_size, cv::Rect crop_region,
                                          cv::Size crop_resize, void* gpu_mat_pointer, int gpu_pitch) {
  // Decode/crop/resize it.  Now we have the GPU Mat pointer
  alprgpusupport_decode_jpeg(gpu_id, (uint8_t*) jpeg_data, jpeg_size, gpu_mat_pointer, gpu_pitch, 1, crop_region.x,
                             crop_region.y, crop_region.width, crop_region.height, crop_resize.width,
                             crop_resize.height);
}

float* AlprGpuSupport::get_gpu_vehicleclassifier_crops(float* vehicle_crop_pointer,
                                                       std::vector<unsigned char*> mat_pointers, int bytes_per_pixel,
                                                       int width, int height, int max_batch_size, int crop_width,
                                                       int crop_height, std::vector<VehicleCropInfo> crop_info,
                                                       float& image_mean, size_t& data_size) {
  if (mat_pointers.size() != crop_info.size())
    std::cout << "Vehicle crop mismatch num crops with images" << std::endl;

  std::vector<int> corners;
  for (VehicleCropInfo & c : crop_info) {
    // Just push the top-left and bottom-right coordinates
    corners.push_back(c.x);
    corners.push_back(c.y);
    corners.push_back(c.width);
    corners.push_back(c.height);
  }
  std::vector<unsigned char*> input_ptr;
  for (int i = 0; i < mat_pointers.size(); i++) {
    input_ptr.push_back(mat_pointers[i]);
  }
  float* data_pointer = NULL;
  alprgpusupport_get_vehicle_classifier_crops(vehicle_crop_pointer, gpu_id, max_batch_size, input_ptr.data(),
                                              bytes_per_pixel, width, height,
                                              mat_pointers.size(), crop_width, crop_height, crop_info.size(),
                                              corners.data(), &image_mean, &data_pointer, &data_size);
  return data_pointer;
}


void * AlprGpuSupport::memory_allocate_gpu_buffer(size_t size) {
  return alprgpusupport_allocate_raw_buffer(gpu_id, size);
}

void* AlprGpuSupport::memory_allocate_gpu_pitched_buffer(int width, int height, int channels, size_t* pitch) {
  return alprgpusupport_allocate_pitched_buffer(gpu_id, width, height, channels, pitch);
}

void AlprGpuSupport::memory_free_gpu_buffer(void * ptr) {
  alprgpusupport_free_raw_buffer(gpu_id, ptr);
}


void* AlprGpuSupport::memory_allocate_pinned_memory_buffer(size_t size) {
  if (library_loaded)
    return alprgpusupport_allocate_pinned_host_buffer(gpu_id, size);
  return malloc(size);
}

void AlprGpuSupport::memory_free_pinned_memory_buffer(void * ptr) {
  if (library_loaded)
    alprgpusupport_free_pinned_host_buffer(gpu_id, ptr);
  else
    free(ptr);
}

void AlprGpuSupport::copy_image_to_gpu(cv::Mat& image, void* gpu_ptr) {
  // Grab the pointer and the size and send it for upload
  const int NUM_CHANNELS = 3;
  alprgpusupport_upload_image(gpu_id, (unsigned char*)image.data, NUM_CHANNELS, image.cols, image.rows, gpu_ptr);
}

void AlprGpuSupport::copy_gpu_image_to_gpu(void* src_gpu_ptr, void* dst_gpu_ptr, int height, int width, int channels) {
  alprgpusupport_upload_image(gpu_id, (unsigned char*)src_gpu_ptr, channels, height, width, dst_gpu_ptr);
}

void AlprGpuSupport::download_image(cv::Mat& dst_mat, void* gpu_ptr, int height, int width, int channels) {
  return download_image(dst_mat, gpu_ptr, height, width, channels, cv::Size(-1, -1));
}

void AlprGpuSupport::download_image(cv::Mat& dst_mat, void* gpu_ptr, int height, int width, int channels, cv::Size resize_target) {
  int64_t imageSize = height * width * channels;
  if (resize_target.width > 0 && resize_target.height > 0) {
    dst_mat.create(resize_target, CV_8UC3);
    alprgpusupport_download_image(gpu_id, dst_mat.data, channels, height, width, gpu_ptr, 1,
                                  resize_target.width, resize_target.height);
  } else {
    dst_mat.create(cv::Size(height, width), CV_8UC3);
    alprgpusupport_download_image(gpu_id, dst_mat.data, channels, height, width, gpu_ptr, 0, -1, -1);
  }
}

void AlprGpuSupport::set_onnxruntime_gpu(void* ort_session_options, bool prefer_tensorrt) {
  int trt_pref = 0;
  if (prefer_tensorrt)
    trt_pref = 1;
  alprgpusupport_set_onnxruntime_gpu(gpu_id, ort_session_options, trt_pref);
}

bool AlprGpuSupport::supports_tensorrt() {
  return alprgpusupport_supports_tensorrt(gpu_id);
}
}  // namespace alpr

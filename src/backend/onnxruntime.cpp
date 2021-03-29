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

#include "onnxruntime.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <inttypes.h>
#include <stdint.h>
#include <onnxruntime/core/session/onnxruntime_c_api.h>
#include <alprsupport/filesystem.h>
#include <alprsupport/timing.h>
#include <opencv2/highgui/highgui.hpp>
#include <alprlog.h>
#include <cmath>
#include <vector>
#include <ctime>
#include <iostream>
#include <chrono>
#include <queue>
#include <iomanip>
#include <utility>
#include <alprsupport/profiler.h>

#ifdef _WIN32
#include <windows.h>
#endif

using std::vector;
using std::pair;
using std::string;

namespace alpr {

// We need to remove these ASAP.
const OrtApi* g_ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
// When we use version > https://github.com/microsoft/onnxruntime/issues/1430
// We can remove this from global namespace.
OrtEnv* g_env;
bool a = g_ort->CreateEnv(ORT_LOGGING_LEVEL_ERROR, "alpr", &g_env);

void OrtCheckStatus(OrtStatus* status) {
  if (status != NULL) {
      const char* msg = g_ort->GetErrorMessage(status);
      ALPR_WARN << msg;
      g_ort->ReleaseStatus(status);
  }
}

void set_omp_to_synchronous() {

}

std::vector<char> read_model(const char* filename, std::string enc_key_name) {

    int fd = open(filename, O_RDONLY);
    if (fd == -1) {
      ALPR_WARN << "Runtime data file not found: " << filename;
      exit(EXIT_FAILURE);
    }
    close(fd);
    std::ifstream file(filename, std::ios::binary | std::ios::ate);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<char> buffer(size);
    if (!file.read(buffer.data(), size)) {
      ALPR_WARN << "Runtime data file not found: " << filename;
      exit(EXIT_FAILURE);
    }
    file.close();
    return buffer;
  
}

AlprONNXRuntime::AlprONNXRuntime(const std::string & model_path, ProcessingProvider proc_type, int gpu_id,
                                 bool pad_to_max, std::string logid, int max_batch_size)
                                 : _proc_type(proc_type), _profile(false), _max_batch_size(max_batch_size),
                                 _pad_to_max(pad_to_max), _input_buffer_manager(max_batch_size, pad_to_max),
                                 _gpu_id(gpu_id), _env(g_env) {
  _alpr_gpu_support = NULL;
  if (proc_type != ORT_CPU) {
    _alpr_gpu_support = AlprGpuSupport::getInstance(_gpu_id);
  }
  set_omp_to_synchronous();
  _input_buffer_manager.SetCudaSupport(_alpr_gpu_support);
  //*************************************************************************
  // initialize enviroment. one enviroment per process
  // enviroment maintains thread pools and other state info
  // OrtCheckStatus(g_ort->CreateEnv(ORT_LOGGING_LEVEL_ERROR, logid.c_str(), &_env));

  // initialize session options if needed
  OrtCheckStatus(g_ort->CreateSessionOptions(&_session_options));
  OrtCheckStatus(g_ort->SetIntraOpNumThreads(_session_options, 1));
#ifndef _WIN32
  if (_profile)
    OrtCheckStatus(g_ort->EnableProfiling(_session_options, "profile"));
#endif

  // Sets graph optimization level
  OrtCheckStatus(g_ort->SetSessionGraphOptimizationLevel(_session_options, ORT_ENABLE_ALL));
  OrtCheckStatus(g_ort->SetIntraOpNumThreads(_session_options, 1));

  switch (proc_type) {
    case ORT_CUDA: {
      this->_alpr_gpu_support->set_onnxruntime_gpu(_session_options, false);
      break;
    }
    case ORT_TRT: {
      this->_alpr_gpu_support->set_onnxruntime_gpu(_session_options, true);
      break;
    }
    case ORT_CPU: {
      break;
    }
    default: {
      ALPR_WARN << "Unexpected processing unit option received";
      exit(EXIT_FAILURE);
    }
  }

  std::vector<char> raw_model = read_model(model_path.c_str(), "edge");
  OrtCheckStatus(g_ort->CreateSessionFromArray(_env, static_cast<void *>(raw_model.data()), raw_model.size(),
                                               _session_options, &_session));

  //*************************************************************************
  // print model input layer (node names, types, shape etc.)
  size_t num_input_nodes;
  OrtStatus* status;
  OrtCheckStatus(g_ort->GetAllocatorWithDefaultOptions(&_allocator));
  OrtCheckStatus(g_ort->SessionGetInputCount(_session, &num_input_nodes));
  // iterate over all input nodes
  for (size_t i = 0; i < num_input_nodes; i++) {
    char* name = NULL;
    OrtCheckStatus(g_ort->SessionGetInputName(_session, i, _allocator, &name));
    // print input node types
    OrtTypeInfo* typeinfo;
    OrtCheckStatus(g_ort->SessionGetInputTypeInfo(_session, i, &typeinfo));
    const OrtTensorTypeAndShapeInfo* tensor_info;
    OrtCheckStatus(g_ort->CastTypeInfoToTensorInfo(typeinfo, &tensor_info));
    ONNXTensorElementDataType type;
    OrtCheckStatus(g_ort->GetTensorElementType(tensor_info, &type));
    // print input shapes/dims
    size_t num_dims;
    OrtCheckStatus(g_ort->GetDimensionsCount(tensor_info, &num_dims));
    std::vector<int64_t> dims(num_dims);
    OrtCheckStatus(g_ort->GetDimensions(tensor_info, reinterpret_cast<int64_t *>(dims.data()), num_dims));

    _input_buffer_manager.RegisterBuffer(i, name, dims, type);
    g_ort->ReleaseTypeInfo(typeinfo);
    OrtCheckStatus(g_ort->AllocatorFree(_allocator, name));
  }

  size_t num_output_nodes;
  status = g_ort->SessionGetOutputCount(_session, &num_output_nodes);
  for (size_t i = 0; i < num_output_nodes; i++) {
    char* name = NULL;
    OrtCheckStatus(g_ort->SessionGetOutputName(_session, i, _allocator, &name));
    _outputs.insert({std::string(name), OutputMeta()});
    // record names, order is important here.
    _output_names.push_back(strdup(name));
    _output_tensors.push_back(nullptr);
    OrtCheckStatus(g_ort->AllocatorFree(_allocator, name));
  }
  g_ort->ReleaseStatus(status);
}

AlprONNXRuntime::~AlprONNXRuntime(void) {
  if (_profile)
    OrtCheckStatus(g_ort->DisableProfiling(_session_options));
  g_ort->ReleaseSession(_session);
  g_ort->ReleaseSessionOptions(_session_options);
  // When we use version > https://github.com/microsoft/onnxruntime/issues/1430
  // uncomment.
  // g_ort->ReleaseEnv(_env);
  for (auto & x : _output_tensors)
    g_ort->ReleaseValue(x);
  for (auto & x : _output_names) {
    free((void *)x);
    x = NULL;
  }
}

bool AlprONNXRuntime::IsCuda(void) {
  return _proc_type != ORT_CPU;
}

// void AlprONNXRuntime::SetInputShape(const std::string & name, const std::vector<int64_t> & dims) {
//   ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "AlprONNXRuntime::SetInputShape");
//   bool updated_shape = _input_buffer_manager.SetTensorDims(name, dims);

//   // if the input shape was update, release any output tensors that the network has allocated.
//   // when the request with the new shape happens, the network will allocate new buffers for output tensors.
//   if (updated_shape) {
//     for (int i = 0; i < _output_tensors.size(); i++) {
//       g_ort->ReleaseValue(_output_tensors[i]);
//       _output_tensors[i] = NULL;
//     }
//   }
//   ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
// }

void AlprONNXRuntime::Infer() {
  ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "AlprONNXRuntime::Infer");
  ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "AlprONNXRuntime::Run");
  auto input_names = _input_buffer_manager.Names();
  auto input_tensors = _input_buffer_manager.Tensors();
  OrtValue** v = _output_tensors.data();
  OrtCheckStatus(g_ort->Run(_session, NULL, input_names.data(), input_tensors.data(), input_tensors.size(),
                            _output_names.data(), _output_names.size(), v));
  ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
  ALPR_PROF_SCOPE_START(alprsupport::Profiler::Get(), "AlprONNXRuntime::Postprocessor");

  for (int i = 0; i < _output_tensors.size(); i++) {
    int is_tensor;
    OrtCheckStatus(g_ort->IsTensor(_output_tensors[i], &is_tensor));
    assert(is_tensor);
    char* _name = NULL;
    OrtCheckStatus(g_ort->SessionGetOutputName(_session, i, _allocator, &_name));
    std::string name(_name);
    OrtTensorTypeAndShapeInfo* tensor_info;
    OrtCheckStatus(g_ort->GetTensorTypeAndShape(_output_tensors[i], &tensor_info));
    ONNXTensorElementDataType type;
    OrtCheckStatus(g_ort->GetTensorElementType(tensor_info, &type));
    size_t num_dims;
    OrtCheckStatus(g_ort->GetDimensionsCount(tensor_info, &num_dims));
    std::vector<int64_t> & dims = _outputs[name].dims;
    dims.resize(num_dims);
    OrtCheckStatus(g_ort->GetDimensions(tensor_info, reinterpret_cast<int64_t *>(dims.data()), num_dims));
    OrtCheckStatus(g_ort->GetTensorMutableData(_output_tensors[i], static_cast<void**>(&_outputs[name].data)));
    OrtCheckStatus(g_ort->AllocatorFree(_allocator, _name));
    g_ort->ReleaseTensorTypeAndShapeInfo(tensor_info);
  }
  ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
  ALPR_PROF_SCOPE_END(alprsupport::Profiler::Get());
}

int AlprONNXRuntime::GetGpuId(void) {
  return _gpu_id;
}

AlprGpuSupport * AlprONNXRuntime::GetGpuSupport(void) {
  return _alpr_gpu_support;
}
}  // namespace alpr

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

#ifndef OPENALPR_BACKEND_ONNXRUNTIME_H_
#define OPENALPR_BACKEND_ONNXRUNTIME_H_

#include <assert.h>
#include <onnxruntime/core/session/onnxruntime_c_api.h>
#include <cmath>
#include <stdlib.h>
#include <stdio.h>
#include <vector>
#include <ctime>
#include <iostream>
#include <chrono>
#include <alprsupport/timing.h>
#include <queue>
#include <iomanip>
#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>
#include <inttypes.h>
#include <stdint.h>
#include <alprlog.h>
#ifdef _WIN32
#include <windows.h>
#include <dlfcn.h>
#else
#include <omp.h>
#endif
#include <alprsupport/streamcrypt/filecryptstream.h>
#include <fcntl.h>
#include "buffer_manager.h"

using std::unordered_map;
using std::vector;
using std::string;
using std::pair;

namespace alpr {

extern const OrtApi* g_ort;
extern OrtEnv* g_env;

void set_omp_to_synchronous();
void OrtCheckStatus(OrtStatus* status);
std::vector<char> read_model(const char* filename, std::string enc_key_name);


typedef enum {
  ORT_CPU,
  ORT_CUDA,
  ORT_TRT
} ProcessingProvider;

struct OutputMeta {
  void * data;
  std::vector<int64_t> dims;
  OutputMeta() : data(NULL) { }
  ~OutputMeta() {
    // *data is owned by ONNXRuntime.
    // if (data)
    //   free(data);
  }
};

class AlprONNXRuntime {
 public:
  int width, height;
  AlprONNXRuntime(const std::string & model_path, ProcessingProvider proc_type, int gpu_id, bool pad_to_max = false,
                  std::string logid = "alpredge", int max_batch_size = 1);
  ~AlprONNXRuntime(void);

  // void SetInputShape(const std::string & name, const std::vector<int64_t> & dims);

  void Infer();
  bool IsCuda(void);
  int GetGpuId(void);
  AlprGpuSupport * GetGpuSupport(void);

  std::vector<int64_t> GetCurInputBufferDims(const std::string & name) {
    std::vector<int64_t> res = _input_buffer_manager.GetCurRawBufferDims(name);
    return res;
  }
  std::vector<int64_t> GetProtoInputBufferDims(const std::string & name) {
    std::vector<int64_t> res = _input_buffer_manager.GetProtoRawBufferDims(name);
    return res;
  }

  template<typename T>
  T * GetInputBuffer(const std::string & name, const std::vector<int64_t> & batch_dims, size_t * buf_size, bool cpu = false) {
    if (batch_dims.size() == 0) {
      ALPR_ERROR << "Got empty batch dims";
      exit(EXIT_FAILURE);
    }
    if (_pad_to_max && _max_batch_size > 0 && _max_batch_size < batch_dims[0]) {
      ALPR_ERROR << "batch_size exceeds max batch_size" << std::endl;
      exit(EXIT_FAILURE);
    }
    if (cpu && _input_buffer_manager.IsGpuBuffer(name))
      _input_buffer_manager.MoveToCpu(name);

    bool updated_shape = _input_buffer_manager.SetTensorDims(name, batch_dims);
    // if the input shape was update, release any output tensors that the network has allocated.
    // when the request with the new shape happens, the network will allocate new buffers for output tensors.
    if (updated_shape) {
      for (int i = 0; i < _output_tensors.size(); i++) {
        g_ort->ReleaseValue(_output_tensors[i]);
        _output_tensors[i] = NULL;
      }
    }

    *buf_size = _input_buffer_manager.GetRawBufferSize(name);
    T * res = static_cast<T *>(_input_buffer_manager.GetRawBuffer(name));
    return res;
  }

  template<typename T>
  T * GetOutputBuffer(const std::string & name, std::vector<int64_t> & dims) {
    dims = _outputs[name].dims;
    return static_cast<T *>(_outputs[name].data);
  }

 private:
  void MakeNetworkInput(const string & node_name, float * buf, size_t buf_size);
  void AllocateInputBuffers();


  OrtEnv* _env;
  OrtSession* _session;
  OrtMemoryInfo* _memory_info;
  OrtSessionOptions* _session_options;

  BufferManager _input_buffer_manager;
  const ProcessingProvider _proc_type;
  int _gpu_id;
  AlprGpuSupport * _alpr_gpu_support;
  const bool _profile;
  const int _max_batch_size;
  const bool _pad_to_max;
  std::vector<const char *> _output_names;
  std::unordered_map<std::string, OutputMeta> _outputs;
  std::vector<OrtValue*> _output_tensors;
  OrtAllocator* _allocator;
};
}  // namespace alpr
#endif  // OPENALPR_BACKEND_ONNXRUNTIME_H_

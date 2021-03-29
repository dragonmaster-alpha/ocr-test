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

#ifdef _WIN32
#include <windows.h>
#endif

using std::vector;
using std::pair;
using std::string;

namespace alpr {


/*
the intention for this class was to be able to allocate/free/cache/invalidate any input and output, and hide
that functionality from onnxruntime.cpp.

For example, when we update one input and this also changes the shape of an output, then we'd want that output
to be invalidated.

The intenntion was for all this to happen inside here. However, this ended up happing in GetInputBuffer and needs
to be moved here being a single class.
*/

BufferManager::BufferManager(int max_batch_size, bool pad_to_max)
                             : _max_batch_size(max_batch_size)
                             , _pad_to_max(pad_to_max)
                             , _is_gpu(false)
                             , _buffer_initialized(false) {}

BufferManager::~BufferManager() {
  for (auto & x : _tensors) {
    g_ort->ReleaseValue(x);
  }
  for (auto & x : _buffers)
    x.second.free();

  for (auto & x : _node_names)
    free(reinterpret_cast<void *>(const_cast<char *>(x)));
}

void BufferManager::SetCudaSupport(AlprGpuSupport* alpr_gpu_support) {
  this->_is_gpu = alpr_gpu_support != NULL;
  this->_alpr_gpu_support = alpr_gpu_support;
}


bool BufferManager::SetTensorDims(const std::string & name, std::vector<int64_t> dims) {
  TensorDesc & desc = GetTensorDesc(name);

  bool dims_equal = desc.DimsAreEqual(dims);
  if (dims_equal) {
    return false;  // doesn't need to update dims.
  }
  if (!desc.CanUpateDims(dims)) {
    ALPR_ERROR << "Bad dimensionn update for buffer `" << name << "'" << std::endl;
    std::cout << "proto_dims: [";
    for (const auto & x : desc.proto_dims)
      std::cout << x << " ";
    std::cout << "]" << std::endl;

    std::cout << "cur_dims: [";
    for (const auto & x : desc.cur_dims)
      std::cout << x << " ";
    std::cout << "]" << std::endl;

    std::cout << "input_dims: [";
    for (const auto & x : dims)
      std::cout << x << " ";
    std::cout << "]" << std::endl;

    exit(EXIT_FAILURE);
  }

  vector<bool> dim_updated(dims.size(), false);
  for (int i = 0; i < dims.size(); i++) {
    dim_updated[i] = dims[i] != desc.cur_dims[i];
  }
  desc.cur_dims = dims;
  desc.free();
  if (_pad_to_max && desc.cur_dims[0] < _max_batch_size)
    desc.cur_dims[0] = _max_batch_size;
  desc.allocate();
  MakeORTdescriptor(desc);

  for (auto & x : _buffers) {
    if (x.first == name) {
      continue;
    } else {
      if (x.second.cur_dims[0] < desc.cur_dims[0]) {
        x.second.free();
        x.second.cur_dims[0] = desc.cur_dims[0];
        x.second.allocate();
        MakeORTdescriptor(x.second);
      }
    }
  }
  desc.allocate();
  MakeORTdescriptor(desc);
  return true;
}


void BufferManager::RegisterBuffer(size_t pos, char * name, std::vector<int64_t> proto_dims, ONNXTensorElementDataType type) {
  if (_node_names.size() != pos) {
    ALPR_WARN << "Got out of order tensor";
    exit(EXIT_FAILURE);
  }
  _node_names.push_back(strdup(name));
  TensorDesc desc(pos, std::vector<int64_t>(proto_dims), type, _alpr_gpu_support, _pad_to_max, _max_batch_size);
  _buffers.insert(std::pair<std::string, TensorDesc>(name, desc));
  _tensors.push_back(NULL);
}

void BufferManager::MoveToCpu(const std::string & name) {
  TensorDesc & desc = GetTensorDesc(name);
  desc.free();
  desc.is_gpu = false;
  if (!desc.allocate()) {
    ALPR_WARN << "Failed to allocate buffer `" << name << "'" << std::endl;
    exit(EXIT_FAILURE);
  }
  MakeORTdescriptor(desc);
}

TensorDesc & BufferManager::GetTensorDesc(const std::string & name) {
  if (!_buffers.count(name)) {
    ALPR_WARN << "BufferManager::Node " << name << " not found";
    exit(EXIT_FAILURE);
  }
  return _buffers[name];
}

size_t BufferManager::GetNodeId(const std::string & name) {
  return GetTensorDesc(name).index;
}

/*
this returns whatever user last set for dims for input `name'.
*/
std::vector<int64_t> BufferManager::GetCurRawBufferDims(const std::string & name) {
  std::vector<int64_t> & d = GetTensorDesc(name).cur_dims;
  std::vector<int64_t> dims(d);
  return dims;
}

/*
this returns whatever ONNX has as dims for input `name'. The intention is for the user
to be able to query the default dimensions of an input.
*/
std::vector<int64_t> BufferManager::GetProtoRawBufferDims(const std::string & name) {
  std::vector<int64_t> dims = GetTensorDesc(name).proto_dims;
  return dims;
}

bool BufferManager::IsGpuBuffer(const std::string & name) {
  return GetTensorDesc(name).is_gpu;
}

void * BufferManager::GetRawBuffer(const std::string & name) {
  return GetTensorDesc(name).buf;
}

size_t BufferManager::GetRawBufferSize(const std::string & name) {
  return GetTensorDesc(name).size;
}

void BufferManager::MakeORTdescriptor(TensorDesc & desc) {
  // free any buffer.
  if (desc.index < _tensors.size()) {
    if (_tensors[desc.index]) {
      g_ort->ReleaseValue(_tensors[desc.index]);
      _tensors[desc.index] = NULL;
    }
  } else {
    ALPR_WARN << "Failed to access input node ";
    exit(EXIT_FAILURE);
  }

  // the cur_dims field has what the user intends, however, GetRealDims returns dimensions
  // after any padding to max_batch_size has been applied.
  auto real_dims = desc.GetRealDims();
  OrtValue* input_tensor = NULL;
  OrtMemoryInfo * mem_info;
  int gpu_id = 0;
  if (desc.is_gpu)
    OrtCheckStatus(g_ort->CreateMemoryInfo("Cuda", OrtDeviceAllocator, gpu_id, OrtMemTypeDefault, &mem_info));
  else
    OrtCheckStatus(g_ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &mem_info));

  OrtCheckStatus(g_ort->CreateTensorWithDataAsOrtValue(mem_info, desc.buf, desc.BufferSizeBytes(),
                                                       real_dims.data(), real_dims.size(),
                                                       desc.elem_type, &input_tensor));
  _tensors[desc.index] = input_tensor;
  int is_tensor;
  OrtCheckStatus(g_ort->IsTensor(input_tensor, &is_tensor));
  assert(is_tensor);
  g_ort->ReleaseMemoryInfo(mem_info);
}
}  // namespace alpr

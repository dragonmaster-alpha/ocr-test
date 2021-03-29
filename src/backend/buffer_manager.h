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

#ifndef OPENALPR_BACKEND_BUFFER_MANAGER_H_
#define OPENALPR_BACKEND_BUFFER_MANAGER_H_

#include <assert.h>
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
#include <alprgpusupport.h>
#include <inttypes.h>
#include <stdint.h>
#ifdef _WIN32
#include <windows.h>
#include <dlfcn.h>
#else
#include <omp.h>
#endif
#include <alprsupport/streamcrypt/filecryptstream.h>
#include <fcntl.h>


using std::unordered_map;
using std::vector;
using std::string;
using std::pair;

namespace alpr {

const std::vector<size_t> _ONNX_element_size = {
  0,   // ONNX_TENSOR_ELEMENT_DATA_TYPE_UNDEFINED,
  sizeof(float),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT,
  sizeof(uint8_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT8
  sizeof(int8_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT8
  sizeof(uint16_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_UINT16
  sizeof(int16_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT16
  sizeof(int32_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT32
  sizeof(int64_t),  // ONNX_TENSOR_ELEMENT_DATA_TYPE_INT64
};
class TensorDesc {
 public:
  const size_t index;
  // proto_dims are the dimensions hardcoded inside the ONNX file.
  // if there's a negative dimension, then it means that this dimension is dynamic.
  const vector<int64_t> proto_dims;
  // cur_dims holds the current dimensions of the tensor,
  // therefore there can't be negative inside this.
  vector<int64_t> cur_dims;
  const bool _pad_to_max;
  const size_t _max_batch_size;
  size_t elem_size;
  AlprGpuSupport* _alpr_gpu_support;
  ONNXTensorElementDataType elem_type;
  void * buf;
  size_t size;
  bool is_gpu;
  // I'm not sure why this is needed
  TensorDesc()
            : index(0), _pad_to_max(false), _max_batch_size(1), buf(NULL) {}

  TensorDesc(size_t index, std::vector<int64_t> dims, ONNXTensorElementDataType type,
            AlprGpuSupport* alpr_gpu_support, bool pad_to_max, size_t max_batch_size)
            : index(index)
            , proto_dims(dims)
            , cur_dims(dims.size(), 0)
            , elem_size(_ONNX_element_size[static_cast<int>(type)])
            , elem_type(type)
            , buf(NULL)
            , size(0)
            , _alpr_gpu_support(alpr_gpu_support)
            , is_gpu(alpr_gpu_support != NULL)
            , _pad_to_max(pad_to_max)
            , _max_batch_size(max_batch_size) {}

  ~TensorDesc() {
    this->free();
  }

  size_t NumElements(vector<int64_t> dims) {
    size_t num_el = 0;
    for (int i = 0; i < dims.size(); i++) {
      auto x = dims[i];
      if (x <= 0) {
        ALPR_ERROR << "Got non-positive dimension for tensor @ index " << i << " ";
        for (auto & t : dims)
          ALPR_WARN << t << " ";
        std::cout << std::endl;
        exit(EXIT_FAILURE);
      }
      if (num_el == 0)
        num_el = x;
      else
        num_el *= x;
    }
    return num_el;
  }
  size_t BufferSizeBytes(void) {
    std::vector<int64_t> dims = this->GetRealDims();
    return elem_size * NumElements(dims);
  }
  vector<int64_t> GetRealDims(void) {
    vector<int64_t> real_dims = cur_dims;
    if (_pad_to_max)
      real_dims[0] = _max_batch_size;
    return real_dims;
  }

  size_t GetNumDims(void) {
    return cur_dims.size();
  }
  /*
  overloading the equality operator could be slightly better idea, 
  but we'd have to wrap lhs_dims into a TensorDesc class.
  */
  bool DimsAreEqual(const std::vector<int64_t>& lhs_dims) {
    if (cur_dims.size() != lhs_dims.size())
      return false;
    for (int i = 0; i < cur_dims.size(); i++) {
      if (cur_dims[i] != lhs_dims[i]) {
        return false;
      }
    }
    return true;
  }

  /*
    this function will check lhs_dims entries against proto_dims
    entries to see if the user tries to change a non-dynamic entry.
  */
  bool CanUpateDims(const std::vector<int64_t>& lhs_dims) {
    if (proto_dims.size() != lhs_dims.size()) {
      return false;
    }
    for (int i = 0; i < proto_dims.size(); i++) {
      if (proto_dims[i] < 0)
        continue;
      if (proto_dims[i] != lhs_dims[i]) {
        ALPR_ERROR << "Tried to update static dimension " << i << " from "
                    << proto_dims[i] << " to " << lhs_dims[i] << std::endl;
        return false;
      }
    }
    return true;
  }

  bool allocate(void) {
    this->free();
    size_t buffer_size = this->BufferSizeBytes();
    void * buffer;
    if (this->is_gpu) {
      buffer = _alpr_gpu_support->memory_allocate_gpu_buffer(buffer_size);
    } else {
      buffer = malloc(buffer_size);
    }
    this->buf = buffer;
    this->size = buffer_size;
    if (!buffer) {
      ALPR_WARN << "Failed to allocate input buffer";
      return false;
    }
    return true;
  }

  void free(void) {
    if (this->buf == NULL)
      return;
    if (this->is_gpu) {
      _alpr_gpu_support->memory_free_gpu_buffer(this->buf);
    } else {
      std::free(this->buf);
    }
    // just in case
    this->buf = NULL;
    this->size = 0;
  }
};


class BufferManager {
 public:
  BufferManager(int max_batch_size, bool pad_to_max);
  ~BufferManager();
  void RegisterBuffer(size_t pos, char * name, vector<int64_t> dims, ONNXTensorElementDataType type);
  void * GetRawBuffer(const string & name);
  size_t GetRawBufferSize(const string & name);
  std::vector<int64_t> GetCurRawBufferDims(const std::string & name);
  std::vector<int64_t> GetProtoRawBufferDims(const std::string & name);
  TensorDesc & GetTensorDesc(const std::string & name);
  bool SetTensorDims(const std::string & name, std::vector<int64_t> dims);

  // void MakeNetworkTensors();
  // void SetBufferDims(const std::string & name, const std::vector<int64_t> & batch_dims);

  vector<const char *> & Names() { return _node_names; }
  vector<OrtValue *> & Tensors() { return _tensors; }
  void SetCudaSupport(AlprGpuSupport* alpr_gpu_support);
  bool IsGpuBuffer(const std::string & name);
  void MoveToCpu(const std::string & name);

 private:
  // void AllocateBuffer(TensorDesc & desc);
  void MakeORTdescriptor(TensorDesc & desc);

  size_t GetNodeId(const std::string & name);
  // void FreeBuffer(TensorDesc & in);
  // keep (_node_names and _tensors) as separate vectors, since we want to
  // keep them in contiguous memory spaces
  vector<const char *> _node_names;
  vector<OrtValue *> _tensors;
  unordered_map<string, TensorDesc> _buffers;

  bool _is_gpu;
  AlprGpuSupport* _alpr_gpu_support;
  const int _max_batch_size;
  const bool _pad_to_max;
  bool _buffer_initialized;
};

}  // namespace alpr
#endif  // OPENALPR_BACKEND_BUFFER_MANAGER_H_

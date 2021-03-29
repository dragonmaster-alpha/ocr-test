/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   frame_queue.cpp
 * Author: mhill
 * 
 * Created on April 30, 2017, 12:44 PM
 */

#include <list>
#include <alprlog.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <alprgpusupport.h>
#include "cuda/alprgpusupport_cudaimpl.h"
#include <numeric>      // std::iota
#include <algorithm>    // std::sort, std::stable_sort
#include <iostream>

const int NUM_CHANNELS = 3;

using namespace std;
namespace alpr
{
  const int RAW_POSITION_UNUSED = -1;

  GpuImageBuffer::GpuImageBuffer(int max_queue_size, int gpu_id) {
    
    this->alpr_gpu_support = AlprGpuSupport::getInstance(gpu_id);
    this->_max_queue_size = max_queue_size;
    this->_gpu_id = gpu_id;
    this->_image_width = -1;
    this->_image_height = -1;
    
    raw_positions.resize(max_queue_size);
    for (uint32_t i = 0; i < _max_queue_size; i++)
    {
      raw_positions[i].gpu_pointer = NULL;
      raw_positions[i].gpu_size = 0;
      raw_positions[i].gpu_pitch = 0;
      raw_positions[i].queue_position = RAW_POSITION_UNUSED;

      raw_positions[i].image_width = 0;
      raw_positions[i].image_height = 0;
      raw_positions[i].image_channels = 0;
      
    }
  }


  GpuImageBuffer::~GpuImageBuffer() {
    for (uint32_t i = 0; i < _max_queue_size; i++)
    { 
      // Clean up GPU memory here
      if (raw_positions[i].gpu_pointer != NULL && raw_positions[i].gpu_size > 0)
      {
        alpr_gpu_support->memory_free_gpu_buffer(raw_positions[i].gpu_pointer);
        raw_positions[i].gpu_size = 0;
        raw_positions[i].gpu_pitch = 0;
        raw_positions[i].queue_position = RAW_POSITION_UNUSED;
      }
    }
  }


  GpuBufferStatus GpuImageBuffer::push(void* gpu_buffer, int image_width, int image_height, int num_channels)
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    int insertion_point = 0;
    GpuBufferStatus status = push_local(image_width, image_height, &insertion_point);
    if (status != GPU_BUFFER_OK)
      return status;

    alpr_gpu_support->copy_gpu_image_to_gpu(gpu_buffer, raw_positions[insertion_point].gpu_pointer, image_width, image_height, num_channels);

    return GPU_BUFFER_OK;
  }

  GpuBufferStatus GpuImageBuffer::push(cv::Mat image)
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    int insertion_point = 0;
    GpuBufferStatus status = push_local(image.cols, image.rows, &insertion_point);
    if (status != GPU_BUFFER_OK)
      return status;

    // Upload the CPU image to the GPU
    alpr_gpu_support->copy_image_to_gpu(image, raw_positions[insertion_point].gpu_pointer);
    
    return GPU_BUFFER_OK;
  }

  GpuBufferStatus GpuImageBuffer::push_local(int image_width, int image_height, int* position_to_copy)
  {
    // If image_width/height is not set, set it now and don't warn
    if (this->_image_width < 0 || this->_image_height < 0)
    {
      this->_image_height = image_height;
      this->_image_width = image_width;
    }

    if (image_width != this->_image_width || image_height != this->_image_height)
    {
      // Check to resize memory or flush buffer
      ALPR_WARN << "Different image size in buffer: " << this->_image_width << "x" << this->_image_height 
                << " -> " << image_width << "x" << image_height;
      this->_image_height = image_height;
      this->_image_width = image_width;
    }

    // Find an insertion point.  Assume this size is managed by frame buffer and will always be correct
    // e.g., this code will not be asked to add to a full array, it will always have a free spot here
    int insertion_point = -1;
    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      if (raw_positions[i].queue_position == RAW_POSITION_UNUSED)
      {
        insertion_point = i;
        break;  
      }
    }

    // Still -- warn in case there's a bug
    if (insertion_point < 0)
    {
      ALPR_WARN << "ERROR -- Unexpected image push on full FrameQueueRawBuffer";
      return GPU_BUFFER_FULL;
    }

    // Make sure we have enough memory
    int pitch_size = alprgpusupport_calculate_pitch(_gpu_id, image_width, image_height, NUM_CHANNELS);

    size_t mem_required = pitch_size * (image_height) * NUM_CHANNELS * sizeof(uint8_t);
    if (mem_required > raw_positions[insertion_point].gpu_size)
    {
      if (raw_positions[insertion_point].gpu_pointer != NULL)
        alpr_gpu_support->memory_free_gpu_buffer(raw_positions[insertion_point].gpu_pointer);
      

      // TODO: If we're using GPU, consider using Pinned memory here
      raw_positions[insertion_point].gpu_size = mem_required;
      raw_positions[insertion_point].gpu_pointer = alpr_gpu_support->memory_allocate_gpu_pitched_buffer(
                                                      image_width, image_height, NUM_CHANNELS, &raw_positions[insertion_point].gpu_pitch);
    }

    raw_positions[insertion_point].image_width = image_width;
    raw_positions[insertion_point].image_height = image_height;
    raw_positions[insertion_point].image_channels = NUM_CHANNELS;

    // Update queue mapping
    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      if (i == insertion_point)
      {
        raw_positions[i].queue_position = 0;
      }
      else if (raw_positions[i].queue_position >= 0)
      {
        raw_positions[i].queue_position += 1;
      }
    }

    *position_to_copy = insertion_point;

    return GPU_BUFFER_OK;
  }
  
  
  GpuBufferStatus GpuImageBuffer::remove_at(int queue_position)
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    bool item_exists = false;
    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      if (raw_positions[i].queue_position == queue_position)
      {
        item_exists = true;
        break;
      }
    }

    if (!item_exists)
      return GPU_BUFFER_ITEM_DOES_NOT_EXIST;

    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      if (raw_positions[i].queue_position == queue_position)
      {
        raw_positions[i].queue_position = RAW_POSITION_UNUSED; 
        raw_positions[i].image_width = 0;
        raw_positions[i].image_height = 0;
        raw_positions[i].image_channels = 0;
      }
      else if (raw_positions[i].queue_position > queue_position)
      {
        // Only subtract the position for queue items that come AFTER our deleted value
        raw_positions[i].queue_position -= 1;
      }
      
    }

    return GPU_BUFFER_OK;
  }

  size_t GpuImageBuffer::size()
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    int count = 0;

    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      if(raw_positions[i].queue_position >= 0)
        count++;
    }

    return count;
  }

  std::vector<size_t> GpuImageBuffer::sort_raw_positions()
  {
    std::vector<RawPositions> local_rp_copy = raw_positions;
    vector<size_t> idx(raw_positions.size());
    iota(idx.begin(), idx.end(), 0);

    stable_sort(idx.begin(), idx.end(), 
        [&local_rp_copy](size_t i1, size_t i2) {return local_rp_copy[i1].queue_position > local_rp_copy[i2].queue_position;});

    return idx;
  }

  std::vector<RawImagePointer> GpuImageBuffer::get_gpu_pointers()
  {

    vector<size_t> idx = sort_raw_positions();
    
    std::vector<RawImagePointer> response_pointers;
    int i = 0;
    while (i < raw_positions.size())
    {
      RawPositions rp = raw_positions[idx[i]];

      if (rp.queue_position < 0)
        break;
      RawImagePointer rip;
      rip.pointer = rp.gpu_pointer;
      rip.size = rp.image_width * rp.image_height * rp.image_channels * sizeof(uint8_t);

      response_pointers.push_back(rip);
      i++;
    }
    return response_pointers;
  }

  RawImagePointer GpuImageBuffer::oldest()
  {

    vector<size_t> idx = sort_raw_positions();
    RawPositions rp = raw_positions[idx[idx.size() - 1]];
    
    RawImagePointer rip;
    rip.pointer = rp.gpu_pointer;
    rip.size = rp.image_width * rp.image_height * rp.image_channels * sizeof(uint8_t);

    return rip;
  }

  RawImagePointer GpuImageBuffer::newest()
  {
    vector<size_t> idx = sort_raw_positions();
    RawPositions rp = raw_positions[idx[0]];
    
    RawImagePointer rip;
    rip.pointer = rp.gpu_pointer;
    rip.size = rp.image_width * rp.image_height * rp.image_channels * sizeof(uint8_t);

    return rip;
  }

  // Debug output for testing
  void GpuImageBuffer::print_buffers()
  {
    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      std::cout << "Memory Position " << i << std::endl;
      if (raw_positions[i].queue_position >= 0)
      {
        std::cout << "  - Queue position: " << raw_positions[i].queue_position << std::endl;
        std::cout << "  - GPU pointer: " << (void*) raw_positions[i].gpu_pointer << std::endl;
        std::cout << "  - GPU memory size: " << raw_positions[i].gpu_size << std::endl;
        std::cout << "  - Image size: " << raw_positions[i].image_width << "x" << raw_positions[i].image_height << " (" << raw_positions[i].image_channels << ")" << std::endl;
      }
    }
  }

  void GpuImageBuffer::clear()
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    for (uint32_t i = 0; i < raw_positions.size(); i++)
    {
      raw_positions[i].queue_position = RAW_POSITION_UNUSED;
      raw_positions[i].image_width = 0;
      raw_positions[i].image_height = 0;
      raw_positions[i].image_channels = 0;
    }
  }
  
  void GpuImageBuffer::upload()
  {
    const std::lock_guard<std::mutex> lock(upload_mutex);
    
    std::vector<RawImagePointer> pointers = get_gpu_pointers();

    if (pointers.size() == 0)
      return;

    vector<uchar*> matpointers;
    for (int i = 0; i < pointers.size(); i++)
      matpointers.push_back((uchar*) pointers[i].pointer);

    alpr_gpu_support->upload_batch(matpointers, NUM_CHANNELS, _image_width, _image_height);


  }
}

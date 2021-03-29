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
#include <alprgpusupport.h>
#include <iostream>


using namespace std;
namespace alpr
{
  const int RAW_POSITION_UNUSED = -1;

  GpuImageBuffer::GpuImageBuffer(int max_queue_size, int gpu_id) {
    ALPR_WARN << "Stub GpuImageBuffer not implemented";
  }


  GpuImageBuffer::~GpuImageBuffer() {

  }


  GpuBufferStatus GpuImageBuffer::push(void* gpu_buffer, int image_width, int image_height, int num_channels)
  {

    ALPR_WARN << "Stub GpuImageBuffer push not implemented";
    return GPU_BUFFER_OK;
  }

  GpuBufferStatus GpuImageBuffer::push(cv::Mat image)
  {
    ALPR_WARN << "Stub GpuImageBuffer push not implemented";
    return GPU_BUFFER_OK;
  }

  GpuBufferStatus GpuImageBuffer::push_local(int image_width, int image_height, int* position_to_copy)
  {
    ALPR_WARN << "Stub GpuImageBuffer push_local not implemented";
    return GPU_BUFFER_OK;
  }
  
  
  GpuBufferStatus GpuImageBuffer::remove_at(int queue_position)
  {
    ALPR_WARN << "Stub GpuImageBuffer remove_at not implemented";
    return GPU_BUFFER_OK;
  }

  size_t GpuImageBuffer::size()
  {
    return 0;
  }

  std::vector<size_t> GpuImageBuffer::sort_raw_positions()
  {
    std::vector<size_t> dummy;
    return dummy;
  }

  std::vector<RawImagePointer> GpuImageBuffer::get_gpu_pointers()
  {
    std::vector<RawImagePointer> dummy;

    return dummy;
  }

  RawImagePointer GpuImageBuffer::oldest()
  {
    RawImagePointer rip;
    return rip;
  }

  RawImagePointer GpuImageBuffer::newest()
  {
    RawImagePointer rip;
    return rip;
  }

  // Debug output for testing
  void GpuImageBuffer::print_buffers()
  {
  }

  void GpuImageBuffer::clear()
  {
  }
  
  void GpuImageBuffer::upload()
  {
    
  }
}
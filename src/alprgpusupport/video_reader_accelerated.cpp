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
#include "opencv2/highgui/highgui.hpp"
#include <iostream>
#include <alprgpusupport.h>
#include "cuda/alprgpusupport_cudaimpl.h"

const int NUM_CHANNELS = 3;

using namespace std;
namespace alpr
{

  VideoReaderAccelerated::VideoReaderAccelerated(std::string video_file, bool use_gpu, int gpu_id) {
    this->_use_gpu = use_gpu;
    this->_gpu_id = gpu_id;
    
    init_vars();

    this->_video_file = video_file;

    cv::VideoCapture* cv = new cv::VideoCapture();
    cv->open(video_file);
    cv->set(cv::CAP_PROP_POS_MSEC, 0);

    cv::Mat first_image;
    _loaded = cv->read(first_image);

    if (!_loaded)
    {
      delete cv;
      return;
    }

    cv->set(cv::CAP_PROP_POS_MSEC, 0);
    
    this->image_width = first_image.cols;
    this->image_height = first_image.rows;

    if (_use_gpu && !_is_jetson)
    {
      this->vidcap_ptr = alprgpusupport_vidcap_initialize(_gpu_id, (char*)video_file.c_str());

      // Allocate enough memory for the image
      this->gpu_data_ptr = AlprGpuSupport::getInstance(gpu_id)->memory_allocate_gpu_pitched_buffer(
                                      image_width, image_height, NUM_CHANNELS, &this->gpu_pitch);

      // Don't need the CPU vidcap anymore once we're done loading it
      delete cv;
    }
    else
    {
      this->vidcap_ptr = cv;

      this->cpu_data_ptr = malloc(image_width * image_height * NUM_CHANNELS * sizeof(uint8_t));
    }
  }

  VideoReaderAccelerated::VideoReaderAccelerated(int gpu_id) {
    this->_use_gpu = true;
    this->_gpu_id = gpu_id;

    init_vars();


  }

  void VideoReaderAccelerated::init_vars()
  {
    this->_video_file = "";
    this->vidcap_ptr = NULL;
    this->gpu_data_ptr = NULL;
    this->cpu_data_ptr = NULL;
    this->nv12_buffer = NULL;
    this->nv12_buffer_pitch = 0;
    this->image_height = 0;
    this->image_width = 0;
    this->stream_format = VIDEO_STREAM_UNKNOWN;


    this->_is_jetson = true;

    // GPU decode is faster, but it doesn't support as many video types
    // Only enable this by env var -- for example if we need to do a speed test
    if (const char* gpu_decode_val = std::getenv("OPENALPR_GPU_DECODE"))
    {
      #ifdef IS_JETSON
      this->_is_jetson = true;
      #elif _WIN32
      // Don't do GPU video decode on Windows either
      this->_is_jetson = true;
      #else
      this->_is_jetson = false;
      #endif
    }

  }

  void* VideoReaderAccelerated::get_data_ptr()
  {
    if (gpu_data_ptr != NULL)
      return gpu_data_ptr;
    return cpu_data_ptr;
  }

  VideoReaderAccelerated::~VideoReaderAccelerated() {
    
    if (_use_gpu && !_is_jetson)
    {
      if (vidcap_ptr != NULL)
        alprgpusupport_vidcap_destroy(_gpu_id, vidcap_ptr);

    }
    if (this->nv12_buffer != NULL)
    {
        alprgpusupport_free_raw_buffer(_gpu_id, this->nv12_buffer);
    }

    if (gpu_data_ptr != NULL)
    {
      alprgpusupport_free_raw_buffer(_gpu_id, gpu_data_ptr);
    }
    if (cpu_data_ptr != NULL)
    {
      delete ((cv::VideoCapture*) vidcap_ptr);
      free(cpu_data_ptr);
    }
  }

  bool VideoReaderAccelerated::is_loaded()
  {
    return _loaded;
  }

  // Starts the video over again at time 0
  void VideoReaderAccelerated::reset_video()
  {
    const std::lock_guard<std::mutex> lock(read_mutex);
    
    // For CPU we can seek.  For GPU we'll have to recreate the video
    if (_use_gpu && !_is_jetson)
    {
      alprgpusupport_vidcap_reload_video(_gpu_id, vidcap_ptr, (char*)this->_video_file.c_str());
    }
    else
    {
      cv::VideoCapture* cv = (cv::VideoCapture*) this->vidcap_ptr;
      cv->set(cv::CAP_PROP_POS_FRAMES, 0);
    }
    
  }

  bool VideoReaderAccelerated::convert_nv12_to_bgr(void* nv12_data_ptr, int img_width, int img_height)
  {

    ensure_gpu_memory(img_width, img_height, VIDEO_STREAM_NV12);

    return alprgpusupport_convert_nv12_to_bgr(_gpu_id, nv12_data_ptr, this->nv12_buffer, this->nv12_buffer_pitch, 
		                       this->gpu_data_ptr, this->gpu_pitch, img_width, img_height) != 0;
  }

  // Returns the size of the image (0 if none)
  bool VideoReaderAccelerated::read_video_frame()
  {
    const std::lock_guard<std::mutex> lock(read_mutex);
    
    if (_use_gpu && !_is_jetson)
    {
      int resp = alprgpusupport_vidcap_read_video(_gpu_id, this->vidcap_ptr, this->gpu_data_ptr, this->gpu_pitch, image_width, image_height, NUM_CHANNELS );
      return resp != 0;
    }
    else
    {
      cv::VideoCapture* cv = (cv::VideoCapture*) this->vidcap_ptr;

      int64_t imageSize = image_width * image_height * NUM_CHANNELS;
      cv::Mat imgData = cv::Mat(imageSize, 1, CV_8U, this->cpu_data_ptr);
      cv::Mat img = imgData.reshape(NUM_CHANNELS, image_height);

      bool has_frame = cv->read(img);

      if (!has_frame)
      {
        // Try to push through it if there's some errors in the stream.  Otherwise, exit.
        for (int count = 0; count < 120; count++)
        { 
          has_frame = cv->read(img);
          if (has_frame)
            break;
        }
  
      } 

      return has_frame;
    }
  }


  bool VideoReaderAccelerated::ensure_gpu_memory(int width, int height, VideoStreamFormat format)
  {

    // Only GPU will use this function
    // Allocate GPU resources lazily (we don't know what the size is until the first packet comes in)

    if (this->image_width != width || this->image_height != height || this->stream_format != format)
    {
      if (this->vidcap_ptr != NULL)
      {
        alprgpusupport_vidcap_destroy(_gpu_id, this->vidcap_ptr);
        this->vidcap_ptr = NULL;
      }
      if (this->gpu_data_ptr != NULL)
      {
        alprgpusupport_free_raw_buffer(_gpu_id, gpu_data_ptr);
        this->gpu_data_ptr = NULL;
      }
      

      this->image_width = width;
      this->image_height = height;
      this->stream_format = format;

      // Allocate enough memory for the image
      this->gpu_data_ptr = AlprGpuSupport::getInstance(_gpu_id)->memory_allocate_gpu_pitched_buffer(
                                      image_width, image_height, NUM_CHANNELS, &this->gpu_pitch);

      if (format == VIDEO_STREAM_NV12)
      {
          this->nv12_buffer = AlprGpuSupport::getInstance(_gpu_id)->memory_allocate_gpu_pitched_buffer(
                                      image_width, image_height, 2, &this->nv12_buffer_pitch);
      }
      
      return true;
    }

    return false;
  }

  bool VideoReaderAccelerated::read_stream_frame(void* stream_packet_data, size_t packet_size, VideoStreamFormat format, int image_width, int image_height)
  {

    bool reallocation = ensure_gpu_memory(image_width, image_height, format);
    if (reallocation)
    {
      this->vidcap_ptr = alprgpusupport_vidcap_initialize_stream(_gpu_id, image_width, image_height, (int) stream_format);
    }

    int resp = alprgpusupport_vidcap_read_stream(_gpu_id, this->vidcap_ptr, this->gpu_data_ptr, this->gpu_pitch, stream_packet_data, packet_size);

    return resp != 0;
  }


}

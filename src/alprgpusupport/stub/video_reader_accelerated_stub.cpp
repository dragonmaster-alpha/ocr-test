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
#include <iostream>
#include <alprgpusupport.h>


using namespace std;
namespace alpr
{

  VideoReaderAccelerated::VideoReaderAccelerated(std::string video_file, bool use_gpu, int gpu_id) {
    ALPR_WARN << "Stub VideoReaderAccelerated not implemented";
  }

  VideoReaderAccelerated::VideoReaderAccelerated(int gpu_id) {
    ALPR_WARN << "Stub VideoReaderAccelerated not implemented";

  }

  VideoReaderAccelerated::~VideoReaderAccelerated() {
    
  }

  bool VideoReaderAccelerated::is_loaded()
  {
    return false;
  }

  // Starts the video over again at time 0
  void VideoReaderAccelerated::reset_video()
  {
    // NOT IMPLEMENTED.  For CPU we can seek.  For GPU we'll have to recreate the video
    ALPR_INFO << "Reset video time not implemented in openalprgpu";
  }


  // Returns the size of the image (0 if none)
  bool VideoReaderAccelerated::read_video_frame()
  {
    return false;
  }

  bool VideoReaderAccelerated::read_stream_frame(void* stream_packet_data, size_t packet_size, VideoStreamFormat format, int image_width, int image_height)
  {
    return false;
  }


  bool VideoReaderAccelerated::convert_nv12_to_bgr(void* nv12_data_ptr, int img_width, int img_height)
  {
    return false;
  }

}

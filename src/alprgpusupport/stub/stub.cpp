//
// Created by mhill on 11/4/18.
//

#ifndef OPENALPRGPU_ALPRGPUSUPPORT_CUDAIMPLSTUB_H
#define OPENALPRGPU_ALPRGPUSUPPORT_CUDAIMPLSTUB_H

#include <cstddef>
#include <stdint.h>
#include <cstring>

#ifdef _WIN32
#define GPUSUPPORTC_DLL_EXPORT __declspec( dllexport )
#else
#define GPUSUPPORTC_DLL_EXPORT __attribute__((visibility("default")))
#endif

extern "C"
{


  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_initialize_gpu(int gpu_id) { }

  GPUSUPPORTC_DLL_EXPORT bool alprgpusupport_has_gpu_support(int gpu_id, char *error_message, int error_buffer_size) { 
    strncpy(error_message, "GPU support is not installed", error_buffer_size);
    return false;
  }
  GPUSUPPORTC_DLL_EXPORT bool alprgpusupport_is_jetson() { return false; }
  
  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_set_max_batch_size(int gpu_id, int max_batch_size) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_destroy(int gpu_id) { }

  // Given a batch of images, copy them to a pinned memory location
  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_upload_batch(int gpu_id,
                                unsigned char **pixelData, int bytesPerPixel,
                                int imgWidth, int imgHeight, int num_images) { }

  // Given a single CPU image and a preallocated location on GPU, upload it
  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_upload_image(int gpu_id,
                                unsigned char* pixelData, int bytesPerPixel,
                                int imgWidth, int imgHeight, void* device_pointer) { }

  // Given a single GPU image pointer, download it to a cv::Mat
  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_download_image(int gpu_id,
                                unsigned char* cpu_pointer, int bytesPerPixel,
                                int imgWidth, int imgHeight, void* device_pointer, 
                                int resize, int target_width, int target_height) { }

  // Encode the JPEG at current batch image_index to the data_pointer (optional) and write size to byte_count
  // Returns a new pointer or the provided pointer
  GPUSUPPORTC_DLL_EXPORT char* alprgpusupport_encode_jpeg(int gpu_id, int image_index, int* byte_count,
                                                          int jpeg_quality, 
                                                          void* data_pointer) { }

  GPUSUPPORTC_DLL_EXPORT char* alprgpusupport_encode_jpeg_from_mat(int gpu_id, 
                                                          void* img_gpu_pointer, int img_pitch, int img_width, int img_height,
                                                          int* byte_count, int jpeg_quality, 
                                                          void* data_pointer) { }

  GPUSUPPORTC_DLL_EXPORT char* alprgpusupport_encode_jpeg_crop(int gpu_id, int image_index, int* byte_count,
                                                          int jpeg_quality, int x, int y, int width, int height,
                                                          int output_width, int output_height) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_decode_jpeg(int gpu_id, uint8_t* jpeg_bytes, int byte_count,
                                                         void* dest_data_pointer, int dest_pitch,
                                                         int crop_it, int x, int y, int w, int h, 
                                                         int resize_width, int resize_height) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_release_jpeg(char* encoded_jpeg) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_get_bgr_gpu_mat(int gpu_id, int image_index, int** device_pointer,
                                                         int* imgWidth, int* imgHeight, int* type, int* step ) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_get_singlechannel_gpu_mat(int gpu_id, int image_index, int channel_index, int** device_pointer,
                                                           int* imgWidth, int* imgHeight, int* type, int* step ) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_get_ocr_crops(int gpu_id, int max_batch_size, 
                                                           int crop_width, int crop_height, int num_crops, 
                                                           float* crop_corner_array, 
                                                           float** device_pointer, size_t* device_length) { }

  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_convert_nv12_to_bgr(int gpu_id,
                                                        void* cpu_nv12_ptr, void* gpu_nv12_ptr, size_t gpu_nv12_pitch,
							void* gpu_img_pointer, size_t gpu_img_pitch,
                                                        int img_width, int img_height) { return 0; }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_get_vehicle_classifier_crops(int gpu_id, int max_batch_size, 
                                                                          unsigned char **pixelData, int bytesPerPixel,
                                                                          int imgWidth, int imgHeight, int num_images,
                                                                          int crop_width, int crop_height, int num_crops, 
                                                                          int* vehicle_rect_array, 
                                                                          float* img_mean,
                                                                          float** device_pointer, size_t* device_length) { }

  // Resizes the images within a batch
  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_resize_batch(int gpu_id,
                                                        int batch_size,
                                                        float * output,
                                                        int width, int height) { }

  GPUSUPPORTC_DLL_EXPORT void * alprgpusupport_allocate_raw_buffer(int gpu_id, size_t size) { }

  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_calculate_pitch(int gpu_id, int width, int height, int channels) { }

  GPUSUPPORTC_DLL_EXPORT void * alprgpusupport_allocate_pitched_buffer(int gpu_id, int width, int height, int channels, size_t* pitch) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_free_raw_buffer(int gpu_id, void * ptr) { }

  GPUSUPPORTC_DLL_EXPORT void * alprgpusupport_allocate_pinned_host_buffer(int gpu_id, size_t size) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_free_pinned_host_buffer(int gpu_id, void * ptr) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_set_onnxruntime_gpu(int gpu_id,
                                                        void* ort_session,
                                                        int prefer_tensorrt) { }

  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_supports_tensorrt(int gpu_id) { return 0; }
  // Video capture functions
  GPUSUPPORTC_DLL_EXPORT void* alprgpusupport_vidcap_initialize(int gpu_id,
                                                        char* filename) { }

  // Initialize stream decoder.  For codec:
  // VIDEO_STREAM_H264  = 0,
  // VIDEO_STREAM_H265 = 1,
  // VIDEO_STREAM_MJPEG = 2
  GPUSUPPORTC_DLL_EXPORT void* alprgpusupport_vidcap_initialize_stream(int gpu_id,
                                                        int image_width, int image_height, int codec) { }

  // returns 0 if there are no more frames to grab
  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_vidcap_read_video(int gpu_id,
                                                        void* vidcap_ptr, void* gpu_img_pointer,
                                                        size_t gpu_img_pitch,
                                                        int img_width, int img_height, int num_channels) { }

  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_vidcap_reload_video(int gpu_id, void* vidcap_ptr, char* filename) { return 0; }

  GPUSUPPORTC_DLL_EXPORT int alprgpusupport_vidcap_read_stream(int gpu_id, void* vidcap_ptr, void* gpu_img_pointer, 
                                                      size_t gpu_img_pitch, 
                                                      void* stream_data, size_t stream_data_size) { }

  GPUSUPPORTC_DLL_EXPORT void alprgpusupport_vidcap_destroy(int gpu_id,
                                                        void* vidcap_ptr) { }
}
#endif //OPENALPRGPU_ALPRGPUSUPPORT_CUDAIMPL_H

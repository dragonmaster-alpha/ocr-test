//
// Created by mhill on 11/3/18.
//

#ifndef ALPRGPUSUPPORTC_H
#define ALPRGPUSUPPORTC_H

#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc.hpp>

#ifdef _WIN32
#define GPUSUPPORT_DLL_EXPORT __declspec( dllexport )
#else
#define GPUSUPPORT_DLL_EXPORT
#endif

namespace alpr {
struct GpuMatInfo {
  bool mat_found;
  int* device_pointer;
  int width;
  int height;
  int type;
  int step;
};

struct OcrCropInfo {
  int image_index;
  float x1, y1, x2, y2, x3, y3, x4, y4;
};

struct VehicleCropInfo {
  int x;
  int y;
  int width;
  int height;
};

class GPUSUPPORT_DLL_EXPORT AlprGpuSupport {
 public:
  // Get or create a global AlprGpuSupport object, one per GPU
  static AlprGpuSupport* getInstance(int gpu_id);

  virtual ~AlprGpuSupport();

  void destroy();

  void set_max_batch_size(int max_batch_size);

  bool has_gpu_support() { return library_loaded; }

  bool is_jetson();

  std::string get_error_reason() { return error_message; }
  std::mutex gpu_init_mutex;
  /**
   * Upload a batch of CPU images to the GPU and persist in global memory
   * @param mat_pointers Array of pointers pointing to the OpenCV Mat.data element
   * @param bytes_per_pixel
   * @param img_width
   * @param img_height
   */
  void upload_batch(std::vector<unsigned char*> mat_pointers, int bytes_per_pixel, int img_width, int img_height);


  /**
   * Provide the image index (from a previously uploaded batch).  This will encode the image and return the JPEG bytes
   * You must call release_jpeg() on the returned jpeg bytes
   */
  char* encode_jpeg(int image_index, size_t& jpeg_size, int quality);

  /**
   * Encode a JPEG using your data pointer.  You must ensure that your data buffer is big enough for the full jpeg.
   */
  char* encode_jpeg(int image_index, void* data_pointer, size_t& jpeg_size, int quality);

  /**
   * Given a GpuMat pointer, encode it to JPEG
   */
  char* encode_jpeg(void* gpu_img_pointer, int pitch, int img_width, int img_height, void* out_data_pointer,
                    size_t& jpeg_size, int quality);

  // Crop the image around rect (crop_region) and return the resized JPEG according to output_size
  char* encode_jpeg_crop(int image_index, cv::Rect crop_region, cv::Size output_size, size_t& jpeg_size, int quality);

  void release_jpeg(char* encoded_jpeg);
  /**
   * Pull the pointer for the GPU Mat from a previously uploaded batch from global memory
   * @param image_index The image index in the batch
   * @return All information needed to load a GpuMat object without any copy
   */
  GpuMatInfo get_gpu_image_pointer_bgr(int image_index);

  GpuMatInfo get_gpu_image_pointer_channel(int image_index, int channel_index);

  /**
   * Transforms the crops on GPU given the input information. 
   * @param data_size The length of the device_pointer data
   * @return a pointer on the GPU to the crop location
   */
  float* get_gpu_ocr_crops(int max_batch_size, int crop_width, int crop_height, std::vector<OcrCropInfo> crop_info,
                           size_t& data_size);

  // Given a big JPEG image, crop a portion of it and resize it.
  // Apply the GpuMat data to the gpu_mat_pointer (must be preallocated)
  // This function is used in alprstream when a group finishes... we just have a JPEG and vehicle region
  void decode_jpeg_and_crop(const char* jpeg_data, size_t jpeg_size, cv::Rect crop_region, cv::Size crop_resize,
                            void* gpu_mat_pointer, int gpu_pitch);

  float* get_gpu_vehicleclassifier_crops(float* vehicle_crop_pointer, std::vector<unsigned char*> mat_pointers,
                                        int bytes_per_pixel, int img_width, int img_height, int max_batch_size,
                                        int crop_width, int crop_height, std::vector<VehicleCropInfo> crop_info,
                                        float& image_mean, size_t& data_size);

  void ResizeBatch(float * output, int batch, int width, int height, int buf_width, int buf_height, cv::Rect mask);
  /**
   * Allocates a CUDA memory buffer of the given size
   * Raw buffer is continuous memory
   * PitchedBuffer is 2d padded memory used for storing images (e.g., GPUMat)
   */
  void * memory_allocate_gpu_buffer(size_t size);
  void * memory_allocate_gpu_pitched_buffer(int width, int height, int channels, size_t* pitch);
  void memory_free_gpu_buffer(void * ptr);

  /**
   * Allocates a page-locked (pinned) memory buffer on the host
   * If GPU is not enabled, this does a normal malloc function call
   */
  void * memory_allocate_pinned_memory_buffer(size_t size);
  void memory_free_pinned_memory_buffer(void * ptr);

  /**
   * Push a CPU image to the GPU.  This is used for background transfer 
   **/                
  void copy_image_to_gpu(cv::Mat& image, void* gpu_pointer);

  /**
   * Copy a pitched GPU image to another memory location on GPU
   */
  void copy_gpu_image_to_gpu(void* src_gpu_pointer, void* dest_gpu_pointer, int image_width, int image_height,
                             int channels);

  /**
   * Download a GPU image to CPU Mat
   */
  void download_image(cv::Mat& destination_mat, void* gpu_pointer, int image_width, int image_height, int channels);
  void download_image(cv::Mat& destination_mat, void* gpu_pointer, int image_width, int image_height, int channels,
                      cv::Size resize_target);

  /**
   * Enable OnnxRuntime GPU provider.  If prefer_tensorrt is set and it has been compiled
   * on this platform, it will use it
   */
  void set_onnxruntime_gpu(void* ort_session_options, bool prefer_tensorrt);
  bool supports_tensorrt();

  /**
   * Flag that indicates whether the images have already been uploaded to the GPU and are ready for use
   * Alpr will check this and if it's set, it will not need to upload the images to GPU
   */
  void set_images_preloaded(bool images_preloaded) { this->_images_preloaded = images_preloaded; }
  bool get_images_preloaded() { return this->_images_preloaded; }

 protected:
  AlprGpuSupport(int gpu_id);

 private:
  std::string error_message;

  // If this flag is set, then the images are already loaded when recognize is called
  // No need to upload
  bool _images_preloaded;

  int last_batch_size;

  int gpu_id;

  void* profiler;

  bool library_loaded;
};
// Constructor will attempt to load dlsym.  If it fails, it will return instantly

enum GpuBufferStatus {
  GPU_BUFFER_OK,
  GPU_BUFFER_FULL,
  GPU_BUFFER_ITEM_DOES_NOT_EXIST
};

struct RawPositions {
  int queue_position;  // Logical position in the queue.  Higher is older, lower is newer

  void* gpu_pointer;
  size_t gpu_size;

  int image_width;
  int image_height;
  int image_channels;
  size_t gpu_pitch;
};

struct RawImagePointer {
  void* pointer;
  size_t size;
};

// Maintains a map of GPU pointers in a queue
// On CPU it's more efficient to just malloc/free memory rather than double-copy
class GPUSUPPORT_DLL_EXPORT GpuImageBuffer {
 public:
  GpuImageBuffer(int max_queue_size, int gpu_id);
  virtual ~GpuImageBuffer();

  // Upload data from a CPU pointer to the GPU buffer
  GpuBufferStatus push(cv::Mat image);

  // Copy data from a GPU pointer to the GPU buffer
  GpuBufferStatus push(void* gpu_buffer, int image_width, int image_height, int num_channels);

  size_t size();
  GpuBufferStatus remove_at(int queue_position);

  // Removes all items from the queue -- does not release memory
  void clear();

  // Upload the buffer to the GPU synchronously
  void upload();

  // Return a pointer to the oldest frame
  RawImagePointer oldest();

  // Return a pointer to the newest frame
  RawImagePointer newest();

  // Returns all active image pointers on the queue ordered by logical position
  // This pointer array is sorted by oldest first
  std::vector<RawImagePointer> get_gpu_pointers();

  // Create and release a pinned (page-locked) pointer on CPU
  // Attaching this to the Mat on CPU enables faster CPU->GPU transfer
  void* create_pinned_pointer(int image_width, int image_height, int channels);
  void release_pinned_pointer();

  cv::Size get_image_size() { return cv::Size(_image_width, _image_height); }

 private:
  AlprGpuSupport* alpr_gpu_support;
  int _max_queue_size;
  int _gpu_id;
  std::mutex upload_mutex;

  int _image_width;
  int _image_height;

  void print_buffers();

  GpuBufferStatus push_local(int image_width, int image_height, int* position_to_copy);

  // Maps logical "queue_position" to the raw positions in the buffer
  // the "raw_position" value will be the logical position in the queue, or -1 (for not used)
  // for example -- 3 queue items with a max of 5
  // [-1, 0, 1, -1, 2]
  // The first logical item is in position 2, the second position 3, the third position 5
  // Two of the positions are unused (-1).  When adding new items, we find the first unused position
  // and enter the logical position.
  // Then update all other indexes (+1 or -1)
  std::vector<RawPositions> raw_positions;

  std::vector<size_t> sort_raw_positions();
};

enum VideoStreamFormat {
  VIDEO_STREAM_UNKNOWN = -1,
  VIDEO_STREAM_H264  = 0,
  VIDEO_STREAM_H265 = 1,
  VIDEO_STREAM_MJPEG = 2,
  VIDEO_STREAM_NV12 = 3    // Image format just used on Jetson
};

// Uses GPU (if available) or CPU to read a video file
class GPUSUPPORT_DLL_EXPORT VideoReaderAccelerated {
 public:
  // Initialize this to read a video file
  VideoReaderAccelerated(std::string video_file, bool use_gpu, int gpu_id);

  // Initialize this for H264 stream decoding
  VideoReaderAccelerated(int gpu_id);

  virtual ~VideoReaderAccelerated();

  bool is_loaded();

  // Given an NV12 image already on GPU, convert it and copy it to data_ptr
  // This function is used on Jetson where BGR is not natively supported so we must convert
  bool convert_nv12_to_bgr(void* nv12_data_ptr, int img_width, int img_height);

  // Starts the video over again at time 0
  void reset_video();

  // Returns true if there are more samples, false otherwise
  bool read_video_frame();

  // Returns true if there are more samples, false otherwise
  bool read_stream_frame(void* stream_packet_data, size_t packet_size, VideoStreamFormat format,
                         int image_width, int image_height);

  void* get_data_ptr();

  cv::Size get_image_size() { return cv::Size(image_width, image_height); }

 private:
  // Ensures the GPU memory is allocated.  If reallocation is needed returns true.  Otherwise returns false
  bool ensure_gpu_memory(int width, int height, VideoStreamFormat format);

  void init_vars();

  bool _use_gpu;
  int _gpu_id;
  bool _loaded;
  bool _is_jetson;

  std::string _video_file;
  std::mutex read_mutex;

  int image_width;
  int image_height;
  VideoStreamFormat stream_format;

  void* vidcap_ptr;

  void* cpu_data_ptr;
  void* gpu_data_ptr;
  size_t gpu_pitch;

  void* nv12_buffer;
  size_t nv12_buffer_pitch;
};
}
#endif /* ALPRFORWARDPASSC_H */

//
// Created by mhill on 1/27/19.
//

#ifndef OPENALPR_FASTJPEG_H
#define OPENALPR_FASTJPEG_H


#include <opencv2/opencv.hpp>

#ifdef _WIN32
#define OPENALPRSTREAMUTIL_DLL_EXPORT __declspec( dllexport )
#else
#define OPENALPRSTREAMUTIL_DLL_EXPORT
#endif

namespace alprsupport
{

  class OPENALPRSTREAMUTIL_DLL_EXPORT FastJpegEncoder {
  public:

    static void encode(cv::Mat& image, int jpeg_quality, std::vector<uchar>& bytes);
    static unsigned char* encode(cv::Mat& image, int jpeg_quality, size_t& bytelength);
    static void free_image(unsigned char* buffer);

  private:
  };

  class OPENALPRSTREAMUTIL_DLL_EXPORT FastJpegDecoder {
  public:

    static cv::Mat decode(std::vector<uchar>& buffer, bool return_bgr=true);

    /// Decode a JPEG from the given bytes.  If return_bgr, use BGR format.  Otherwise use RGB
    static cv::Mat decode(unsigned char* bytes, size_t bytelength, bool return_bgr=true);

  private:

  };
}

#endif //OPENALPR_FASTJPEG_H

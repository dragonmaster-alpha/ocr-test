
#include "fastjpeg.h"

#include <turbojpeg.h>
#include <opencv2/imgproc/imgproc.hpp>

namespace alprsupport
{


  void FastJpegEncoder::encode(cv::Mat& image, int jpeg_quality, std::vector<uchar>& bytes)
  {

    size_t bytelength;
    unsigned char* data = encode(image, jpeg_quality, bytelength);

    bytes.resize(bytelength);
    memcpy(&bytes[0], data, bytelength);
    //std::copy(data, data+bytelength, bytes.begin());

    //std::vector<uchar> response_buffer(data, data+bytelength);
    free_image(data);

    //return response_buffer;
  }
  unsigned char* FastJpegEncoder::encode(cv::Mat& image, int jpeg_quality, size_t& bytelength)
  {
    int _width = image.cols;
    int _height = image.rows;
    bytelength = 0;
    unsigned char* _compressedImage = NULL; //!< Memory is allocated by tjCompress2 if bytelength == 0


    int jpeg_conversionmode = TJPF_BGR;
    int jpeg_samplemode = TJSAMP_422;

    if (image.channels() == 1)
    {
      jpeg_conversionmode = TJPF_GRAY;
      jpeg_samplemode = TJSAMP_GRAY;
    }
    else if( image.channels() == 3 )
    {
      jpeg_conversionmode = TJPF_BGR;
      jpeg_samplemode = TJSAMP_422;
    }
    else if( image.channels() == 4 )
    {
      jpeg_conversionmode = TJPF_BGRX;
      jpeg_samplemode = TJSAMP_422;
    }

    tjhandle _jpegCompressor = tjInitCompress();

    tjCompress2(_jpegCompressor, image.data, _width, 0, _height, jpeg_conversionmode,
                &_compressedImage, (long unsigned int*) &bytelength, jpeg_samplemode, jpeg_quality,
                TJFLAG_FASTDCT  );

    tjDestroy(_jpegCompressor);

    return _compressedImage;
  }

  void FastJpegEncoder::free_image(unsigned char* buffer) {
    //to free the memory allocated by TurboJPEG (either by tjAlloc(), 
    //or by the Compress/Decompress) after you are done working on it:
    tjFree(buffer);
  }


  cv::Mat FastJpegDecoder::decode(std::vector<uchar>& buffer, bool return_bgr)
  {
    return decode(buffer.data(), buffer.size(), return_bgr);
  }

  cv::Mat FastJpegDecoder::decode(unsigned char* bytes, size_t bytelength, bool return_bgr)
  {
    const int COLOR_COMPONENTS = 3;


    int jpegSubsamp, width, height;

    tjhandle _jpegDecompressor = tjInitDecompress();

    tjDecompressHeader2(_jpegDecompressor, bytes, bytelength, &width, &height, &jpegSubsamp);

    //std::cout << "JPEG Size: " << width << "x" << height << std::endl;

    size_t totalpixels = width*height*COLOR_COMPONENTS;
    unsigned char* outbuffer = (unsigned char*) malloc(totalpixels);

    int pixel_format = TJPF_BGR;
    if (!return_bgr)
      pixel_format = TJPF_RGB;
    tjDecompress2(_jpegDecompressor, bytes, bytelength, outbuffer, width, 0/*pitch*/, height, pixel_format, TJFLAG_FASTDCT);

    cv::Mat outImg = cv::Mat(height,width, CV_8UC3, outbuffer).clone();
    //cv::Mat outImg(height,width,CV_8UC3,&outbuffer[0]);

    free(outbuffer);

    tjDestroy(_jpegDecompressor);
    //cv::Mat outImg;
    //cv::cvtColor(tempMat,outImg, CV_RGB2BGR);

    return outImg;
  }

}
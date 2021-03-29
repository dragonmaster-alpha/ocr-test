#include <assert.h>
#include <math.h>
#include <fstream>
#include <iostream>
#include <iostream>
#include <memory>
#include <set>
#include <sstream>
#include <vector>
#include <ctime>
#include <string>
#include "tclap/CmdLine.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <iostream>
#include "ocr.h"

using namespace alpr;
using namespace std;

int main(int argc, char **argv) {
  std::vector<string> filenames;
  std::string country;
  bool tracing_enabled = false;
  int iterations = 1;
  int duplicates = 1;

  TCLAP::CmdLine cmd("AlprOCR Command Line Utility", ' ', "1.0.0");
  TCLAP::UnlabeledMultiArg<string>  fileArg("image_file", "Image containing license plates", true, "", "image_file_path");

  TCLAP::ValueArg<int> iterationsArg("i","iterations","Number of iterations to run for the batches. Default=1",false, 1 ,"iterations");
  TCLAP::ValueArg<string> countryArg("c","country","country to use for OCR. Default=us",false, "us" ,"country");
  TCLAP::ValueArg<int> duplicatesArg("d","duplicates","Number of times to repeat image. Default=1",false, 1 ,"duplicates");

  try {
    cmd.add(fileArg);
    cmd.add(countryArg);
    cmd.add(iterationsArg);
    cmd.add(duplicatesArg);

    if (cmd.parse(argc, argv) == false) {
      // Error occurred while parsing. Exit now.
      return 1;
    }

    filenames = fileArg.getValue();
    country = countryArg.getValue();
    iterations = iterationsArg.getValue();
    duplicates = duplicatesArg.getValue();

    if (duplicates > 1) {
      if (filenames.size() != 1) {
        std::cerr << "Use of duplicates arg must be paired with single image file only" << std::endl;
        return 1;
      }
      for (int d = 0; d < duplicates - 1; d++) {
        filenames.push_back(filenames[0]);
      }
    }
  } catch (TCLAP::ArgException &e) {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }

  // Setup logging to console
  AlprLog::instance()->setParameters("alpr", false, "", 0, 0);
  AlprLog::instance()->setLogLevel(ALPRINFO);

  Config config(country, "", "");


  Ocr alpr_ocr(&config);

  if (!alpr_ocr.initialized()) {
    cout << "Error initializing library";
    return 1;
  }

  std::vector<cv::Mat> image_batch;
  vector<OcrRequestCrop> crop_requests;

  for (uint32_t i = 0; i < filenames.size(); i++) {
    string fname = filenames[i];
    cout << "Processing: " << fname << endl;
    cv::Mat img = cv::imread(fname, cv::IMREAD_COLOR);
    if (img.rows > 250 || img.cols > 250)
      cout << "The imageinput crop should match the size of the network.  This image seems too large" << std::endl;
    image_batch.push_back(img);

    OcrRequestCrop crop_request;
    crop_request.ideal_height = img.rows;
    crop_request.ideal_width = img.cols;
    crop_request.corner_points = {0, 0,
                                  (float)crop_request.ideal_width, 0,
                                  (float)crop_request.ideal_width, (float)crop_request.ideal_height,
                                  0, (float)crop_request.ideal_height};
    crop_request.image_index = i;
    crop_requests.push_back(crop_request);
  }


  for (int i = 0; i < iterations; i++) {
    const clock_t begin_time = clock();
    std::vector<OcrResult> alprcharsvec = alpr_ocr.recognize_batch(image_batch, crop_requests);
    std::cout << "Processing time (" << image_batch.size() << " crops): "
              << static_cast<float>(clock() - begin_time) / CLOCKS_PER_SEC << std::endl;
    for (OcrResult alprchars : alprcharsvec) {
      if (alprchars.provinces.size() > 0)
        cout << "  Region: " << alprchars.provinces[0].regioncode << " : " << alprchars.provinces[0].confidence << endl;
      for (uint16_t z = 0; z < alprchars.characters.size(); z++) {
        cout << " : " << alprchars.characters[z].char_index << " - " << alprchars.characters[z].letter
             << " - " << alprchars.characters[z].confidence << endl;
      }
    }
  }


  return 0;
}

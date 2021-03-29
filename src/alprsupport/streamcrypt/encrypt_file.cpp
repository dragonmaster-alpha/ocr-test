
#include <cstdio>
#include <sstream>
#include <iostream>
#include <fstream>
#include "filecryptstream.h"
#include "../tclap/CmdLine.h"
#include "../filesystem.h"

using namespace std;


int main( int argc, const char** argv )
{
  
  TCLAP::CmdLine cmd("OpenAlpr Encrypt Decrypt Utility", ' ', "1.0");

  TCLAP::UnlabeledValueArg<std::string>  outputFileArg( "output_file", "Output file", true, "", "output_file_path"  );
  
  TCLAP::ValueArg<std::string> inputFileArg("i","input_file","Input file to enc/dec",true, "" ,"input_file");
  TCLAP::ValueArg<std::string> keyArg("k","key","Key used for decryption (e.g., ocr, edge, etc). ", true, "" ,"key");

  std::string output_filename;
  std::string input_filename;
  std::string keystr;
  
  try
  {
    cmd.add( outputFileArg );
    cmd.add( inputFileArg );
    cmd.add( keyArg );

    if (cmd.parse( argc, argv ) == false)
    {
      // Error occurred while parsing.  Exit now.
      return 1;
    }
    
    output_filename = outputFileArg.getValue();
    input_filename = inputFileArg.getValue();
    keystr = keyArg.getValue();
    
  }
  catch (TCLAP::ArgException &e)    // catch any exceptions
  {
    std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
    return 1;
  }

  uint8_t key[32];
  uint8_t iv[8];
  if (keystr == "edge")
  {
    for (unsigned int i = 0; i < 32; i++)
      key[i] = (i % 5) * (3 % (i + 2)) * (i % 5) + i;

    for (unsigned int i = 0; i < 8; i++)
      iv[i] = (i % 7) * (7 % (i + 5)) * ((i % 4) + i) + i;
  }
  else if (keystr == "ocr")
  {
    for (unsigned int i = 0; i < 32; i++)
      key[i] = (i % 5) * (2 % (i + 3)) * (i % 6) + i;

    for (unsigned int i = 0; i < 8; i++)
      iv[i] = (i % 6) * (3 % (i + 9)) * ((i % 3) + i) + i;
  }
  else if (keystr == "classifier")
  {
    for (unsigned int i = 0; i < 32; i++)
      key[i] = (i % 5) * (3 % (i + 2)) * (i % 5) + i;

    for (unsigned int i = 0; i < 8; i++)
      iv[i] = (i % 7) * (7 % (i + 5)) * ((i % 4) + i) + i;
  }
  else
  {
    std::cout << "Invalid key type (" << keystr << ") options are: edge, ocr, classifier" << std::endl;
  }

//  if (!alprsupport::fileExists(input_filename.c_str()))
//  {
//    std::cout << "File does not exist: " << input_filename << std::endl;
//    exit(1);
//  }

  alprsupport::FileCryptStream crypt(key, iv);

  std::ifstream t(input_filename.c_str(), std::ios::binary);
  std::string str((std::istreambuf_iterator<char>(t)),
                  std::istreambuf_iterator<char>());

  string encrypted = crypt.encrypt_decrypt(str);

  std::ofstream out(output_filename.c_str(), std::ios::binary);

  out << encrypted;
  out.close();

  return 0;
}

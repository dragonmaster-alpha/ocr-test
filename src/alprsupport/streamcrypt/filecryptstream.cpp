 /*************************************************************************
 * OPENALPR CONFIDENTIAL
 * 
 *  Copyright 2018 OpenALPR Technology, Inc.
 *  All Rights Reserved.
 * 
 * NOTICE:  All information contained herein is, and remains
 * the property of OpenALPR Technology Incorporated. The intellectual
 * and technical concepts contained herein are proprietary to OpenALPR  
 * Technology Incorporated and may be covered by U.S. and Foreign Patents.
 * patents in process, and are protected by trade secret or copyright law.
 * Dissemination of this information or reproduction of this material
 * is strictly forbidden unless prior written permission is obtained
 * from OpenALPR Technology Incorporated.
 */

#include "filecryptstream.h"
#include "filecryptstream_c.h"

#include <iterator>
#include <stdio.h>
#include <memory.h>
#include "salsa20.h"

using namespace std;

namespace alprsupport
{

 OPENALPRSUPPORT_DLL_EXPORT  FileCryptStream::FileCryptStream(uint8_t* enckey, uint8_t* enciv) {
    memcpy(this->key, enckey, 32);
    memcpy(this->iv, enciv, 8);
  }


  OPENALPRSUPPORT_DLL_EXPORT FileCryptStream::~FileCryptStream() {
    //delete ((CryptoPP::Salsa20::Encryption*) salsa);
  }

  std::vector<uint8_t> readFile(const char* filename)
  {
      // open the file:
      std::ifstream file(filename, std::ios::binary);

      // Stop eating new lines in binary mode!!!
      file.unsetf(std::ios::skipws);

      // get its size:
      std::streampos fileSize;

      file.seekg(0, std::ios::end);
      fileSize = file.tellg();
      file.seekg(0, std::ios::beg);

      // reserve capacity
      std::vector<uint8_t> vec;
      vec.reserve(fileSize);

      // read the data:
      vec.insert(vec.begin(),
                 std::istream_iterator<uint8_t>(file),
                 std::istream_iterator<uint8_t>());

      file.close();
      return vec;
  }

  OPENALPRSUPPORT_DLL_EXPORT std::string FileCryptStream::encrypt_decrypt(std::string data) {
    uint8_t *convertedBytes = (uint8_t *) data.c_str();

    s20_status_t success = s20_crypt(key, S20_KEYLEN_256, iv, 0, convertedBytes, data.length());
    
    if (success == S20_SUCCESS)
    {
      std::string response((const char*)convertedBytes, data.length());
      return response;
    }
    return "";

  }


  OPENALPRSUPPORT_DLL_EXPORT std::vector<char> FileCryptStream::encrypt_decrypt_vec(std::string filename) {
    vector<uint8_t> cyphertext = readFile(filename.c_str());
    return encrypt_decrypt_vec(cyphertext);
  }

  OPENALPRSUPPORT_DLL_EXPORT std::vector<char> FileCryptStream::encrypt_decrypt_vec(vector<uint8_t> data)
  {
    uint8_t *convertedBytes = (uint8_t *) data.data();

    s20_status_t success = s20_crypt(key, S20_KEYLEN_256, iv, 0, convertedBytes, data.size());
    
    if (success == S20_SUCCESS)
    {
       std::vector<char> plaintextVec(convertedBytes, convertedBytes+data.size());
      return plaintextVec;
    }
    return std::vector<char>() ;
    
  }

  OPENALPRSUPPORT_DLL_EXPORT char* FileCryptStream::encrypt_decrypt_c(const char* data, size_t size)
  {
    char* convertedBytes = (char*) malloc(size);

    s20_status_t success = s20_crypt(key, S20_KEYLEN_256, iv, 0, (uint8_t*) convertedBytes, size);

    if (success == S20_SUCCESS)
    {
      return convertedBytes;
    }
    return "";
  }
}


 OPENALPRSUPPORT_DLL_EXPORT char* decrypt_filestream(const char* path, size_t* size, uint8_t* enckey, uint8_t* enciv)
 {
     alprsupport::FileCryptStream fcs(enckey, enciv);
     std::vector<char> decrypted_data = fcs.encrypt_decrypt_vec(path);

     char* response = (char*) malloc(decrypted_data.size());
     std::copy(decrypted_data.begin(), decrypted_data.end(), response);
     *size = decrypted_data.size();

     return response;
 }

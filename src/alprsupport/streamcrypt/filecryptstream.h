

#ifndef OPENALPR_FILECRYPTSTREAM_H
#define	OPENALPR_FILECRYPTSTREAM_H

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../exports.h"

namespace alprsupport
{
  class FileCryptStream {
  public:
    OPENALPRSUPPORT_DLL_EXPORT FileCryptStream(uint8_t* enckey, uint8_t* enciv);
    OPENALPRSUPPORT_DLL_EXPORT virtual ~FileCryptStream();

    OPENALPRSUPPORT_DLL_EXPORT std::string encrypt_decrypt(std::string data);
    OPENALPRSUPPORT_DLL_EXPORT char* encrypt_decrypt_c(const char* data, size_t size);
    OPENALPRSUPPORT_DLL_EXPORT std::vector<char> encrypt_decrypt_vec(std::string filename);
    OPENALPRSUPPORT_DLL_EXPORT std::vector<char> encrypt_decrypt_vec(std::vector<unsigned char> data);

  private:

    // Used for Cryptopp version
    void* salsa;

    uint8_t key[32];
    uint8_t iv[8];

  };
}
#endif	/* OPENALPR_FILECRYPTSTREAM_H */


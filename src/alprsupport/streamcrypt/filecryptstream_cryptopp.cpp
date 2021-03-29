/*************************************************************************
* OPENALPR CONFIDENTIAL
*
*  Copyright 2016 OpenALPR Technology, Inc.
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
#include <cryptopp/salsa.h>

using namespace CryptoPP;
using namespace std;


namespace alprsupport {

    static std::vector<char> encrypt_decrypt_raw(char *ciphertextBytes, size_t cyphertextSize, void* salsa, uint8_t* enckey, uint8_t* enciv);

    FileCryptStream::FileCryptStream(uint8_t *enckey, uint8_t *enciv) {

        salsa = new CryptoPP::Salsa20::Encryption();

        memcpy(this->key, enckey, 32);
        memcpy(this->iv, enciv, 8);

    }


    FileCryptStream::~FileCryptStream() {
        delete ((CryptoPP::Salsa20::Encryption *) salsa);
    }


    std::string FileCryptStream::encrypt_decrypt(std::string plaintext) {
        byte *plaintextBytes = (byte *) plaintext.c_str();
        byte *ciphertextBytes = new byte[plaintext.length()];

        CryptoPP::Salsa20::Encryption *salsa_ptr = (CryptoPP::Salsa20::Encryption *) salsa;
        salsa_ptr->SetKeyWithIV(key, 32, iv);

        salsa_ptr->ProcessData(ciphertextBytes, plaintextBytes, plaintext.length());

        std::string response((const char *) ciphertextBytes, plaintext.length());

        delete[] ciphertextBytes;
        return response;

    }


    std::vector<char> FileCryptStream::encrypt_decrypt_vec(std::string filename) {

        // open the file:
        std::ifstream file(filename.c_str(), std::ios::binary);

        // Stop eating new lines in binary mode!!!
        file.unsetf(std::ios::skipws);

        // get its size:
        std::streampos fileSize;

        file.seekg(0, std::ios::end);
        fileSize = file.tellg();
        file.seekg(0, std::ios::beg);


        char *buffer = new char[fileSize];
        file.read(buffer, fileSize);
        file.close();

        std::vector<char> response = encrypt_decrypt_raw(buffer, fileSize, salsa, key, iv);
        delete[] buffer;

        return response;
    }

    std::vector<char> FileCryptStream::encrypt_decrypt_vec(vector<unsigned char> cyphertext) {
        return encrypt_decrypt_raw((char *) cyphertext.data(), cyphertext.size(), salsa, key, iv);
    }

    std::vector<char> encrypt_decrypt_raw(char *ciphertextBytes, size_t cyphertextSize, void* salsa, uint8_t* key, uint8_t* iv) {
        byte *plaintextBytes = (byte*) malloc(cyphertextSize);

        CryptoPP::Salsa20::Encryption *salsa_ptr = (CryptoPP::Salsa20::Encryption *) salsa;

        salsa_ptr->SetKeyWithIV(key, 32, iv);

        salsa_ptr->ProcessData(plaintextBytes, (byte *) ciphertextBytes, cyphertextSize);

        std::vector<char> plaintextVec(plaintextBytes, plaintextBytes + cyphertextSize);

        //  plaintextStr.reserve(cyphertext.length() + 1);
        //
        //  plaintextStr.append((char *) plaintextBytes, cyphertext.length());

        free(plaintextBytes);
        return plaintextVec;
    }

  OPENALPRSUPPORT_DLL_EXPORT char* FileCryptStream::encrypt_decrypt_c(const char* data, size_t size)
  {
      char* ciphertextBytes = (char*) malloc(size);

      CryptoPP::Salsa20::Encryption *salsa_ptr = (CryptoPP::Salsa20::Encryption *) salsa;
      salsa_ptr->SetKeyWithIV(key, 32, iv);

      salsa_ptr->ProcessData((byte*) ciphertextBytes, (byte*) data, size);

      return ciphertextBytes;
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
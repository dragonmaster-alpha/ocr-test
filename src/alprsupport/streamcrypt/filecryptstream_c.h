/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   test.h
 * Author: mhill
 *
 * Created on May 9, 2017, 9:50 PM
 */

#ifndef FILECRYPTSTREAM_C_H
#define FILECRYPTSTREAM_C_H

#include <stdint.h>
#include "../exports.h"
#ifdef __cplusplus
extern "C" {
#endif


OPENALPRSUPPORT_DLL_EXPORT char* decrypt_filestream(const char* path, size_t* size, uint8_t* enckey, uint8_t* enciv);

#ifdef __cplusplus
}
#endif

#endif /* FILECRYPTSTREAM_C_H */



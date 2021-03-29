/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#ifndef CPU_DETECT_H
#define CPU_DETECT_H

#include <iostream>
#include <ctime>
#include <stdint.h>
#include "exports.h"

namespace alprsupport
{
  #define BIT_MASK(a, b) (((unsigned) -1 >> (31 - (b))) & ~((1U << (a)) - 1))

  #define HAS_BIT(val, bit) ((val >> bit) & 1)

  /*
   * struct to hold values copied from the extended 32-bit registers
   */
  typedef struct {
          uint32_t eax;
          uint32_t ebx;
          uint32_t ecx;
          uint32_t edx;
  } e_registers_t;

  /*
   * struct for CPU classification values
   */
  typedef struct {
      int stepping;
      int model;
      int family;
      int processor_type;
      int extended_model;
      int extended_family;
  } cpu_classifiers_t;


  typedef struct {
    e_registers_t reg0, reg1, reg7;
    char vendor[32], brand[64];
    int stepping, model, family, processor_type;
    int extended_model, extended_family;
    int model_display, family_display, signature;
    int max_basic_arg;
    int has_mmx, has_sse, has_sse2, has_sse3, has_3dnow, has_ssse3;
    int has_sse4_1, has_sse4_2;
    int has_reg7;  /* True if cpuid supports cpuid(7) */
    int supports_avx, supports_avx2;
  } cpu_info_t;

  int collect_info(cpu_info_t *info);

  OPENALPRSUPPORT_DLL_EXPORT int has_avx(cpu_info_t * info);
  OPENALPRSUPPORT_DLL_EXPORT int has_sse(cpu_info_t * info);
  OPENALPRSUPPORT_DLL_EXPORT cpu_info_t * cpu_detect(void);
}



#endif /* CPU_DETECT_H */


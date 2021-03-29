#include "cpu_detect.h"

#include <memory>

#ifdef __ANDROID__
#include <cpu-features.h>
#endif

#define HAS_FEATURE(features,feature) (((features) & (feature)) == (feature))

namespace alprsupport {
  int has_sse(cpu_info_t* info) {
    return info->has_sse4_2;
  }

  int has_avx(cpu_info_t* info) {
    return info->supports_avx;
  }

  cpu_info_t* cpu_detect() {
    cpu_info_t * info = (cpu_info_t *)malloc(sizeof(cpu_info_t));
    if (!info) {
      return nullptr;
    }

    memset(info, 0, sizeof(cpu_info_t));

    auto family = android_getCpuFamily();
    uint64_t features = android_getCpuFeatures();

    info->has_ssse3 = HAS_FEATURE(features, ANDROID_CPU_X86_FEATURE_SSSE3);
    info->has_sse4_1 = HAS_FEATURE(features, ANDROID_CPU_X86_FEATURE_SSE4_1);
    info->has_sse4_2 = HAS_FEATURE(features, ANDROID_CPU_X86_FEATURE_SSE4_2);
    info->supports_avx = HAS_FEATURE(features, ANDROID_CPU_X86_FEATURE_AVX);
    info->supports_avx2 = HAS_FEATURE(features, ANDROID_CPU_X86_FEATURE_AVX2);

    return info;
  }
};
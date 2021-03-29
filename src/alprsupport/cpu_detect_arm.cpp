#include "cpu_detect.h"

namespace alprsupport {

int has_avx(cpu_info_t * info) { return false; }
int has_sse(cpu_info_t * info) { return false; }
cpu_info_t * cpu_detect(void) {}

};
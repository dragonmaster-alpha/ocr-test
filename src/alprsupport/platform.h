#ifndef OPENALPR_PLATFORM_H
#define OPENALPR_PLATFORM_H

#include <string.h>
#include <sstream>
#include <thread>
#include <mutex>
#include "exports.h"

#ifdef WINDOWS
	#include <windows.h>
#else
	#include <unistd.h>
#endif

namespace alprsupport {

  static std::mutex global_model_init_mutex;

  OPENALPRSUPPORT_DLL_EXPORT void sleep_ms(int sleepMs);

  OPENALPRSUPPORT_DLL_EXPORT std::string getExeDir();

  OPENALPRSUPPORT_DLL_EXPORT std::string get_hostname();

  /// Set the priority of a thread.  Valid priority value is -1
  OPENALPRSUPPORT_DLL_EXPORT bool set_thread_high_priority(std::thread &th);
  OPENALPRSUPPORT_DLL_EXPORT  bool set_current_thread_high_priority();
}

#endif //OPENALPR_PLATFORM_H

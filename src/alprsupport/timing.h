#ifndef OPENALPR_TIMING_H
#define OPENALPR_TIMING_H

#include <iostream>
#include <ctime>
#include <stdint.h>
#include "exports.h"



#ifdef _WIN32
    // Import windows only stuff
	#include <windows.h>
	#include <sys/timeb.h>  

	#define timespec timeval

#else
    #include <sys/time.h>
#endif

// Support for OS X
#if defined(__APPLE__) && defined(__MACH__)
#include <mach/clock.h>
#include <mach/mach.h>
#endif

namespace alprsupport
{

	OPENALPRSUPPORT_DLL_EXPORT void getTimeMonotonic(timespec* time);
  
	OPENALPRSUPPORT_DLL_EXPORT  double diffclock(timespec time1,timespec time2);

	OPENALPRSUPPORT_DLL_EXPORT int64_t getEpochTimeMs();

}

#endif

#include "platform.h"
#include "exports.h"

#ifdef _WIN32
	
#else
	#include <sched.h>
#endif

namespace alprsupport
{

  OPENALPRSUPPORT_DLL_EXPORT void sleep_ms(int sleepMs)
  {
          #ifdef _WIN32
                  Sleep(sleepMs);
          #else
                  usleep(sleepMs * 1000);   // usleep takes sleep time in us (1 millionth of a second)
          #endif
  }
  

  void fix_utf8_string(std::string& str)
  {
  }

  OPENALPRSUPPORT_DLL_EXPORT std::string get_hostname() {
    const int BUFFER_SIZE = 512;
    char host_buffer[BUFFER_SIZE];
    gethostname(host_buffer, BUFFER_SIZE);

    // Ensure the hostname is represented as valid UTF-8
    std::string raw_val = std::string(host_buffer);


    return raw_val;
  }

  OPENALPRSUPPORT_DLL_EXPORT std::string getExeDir()
  {
          #ifdef _WIN32

            char path[FILENAME_MAX];
            HMODULE hm = NULL;

            if (!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                (LPCSTR)sleep_ms,
                &hm))
            {
                std::ostringstream oss;
                oss << "GetModuleHandle returned " << GetLastError();
                throw std::runtime_error(oss.str());
                
            }
            GetModuleFileNameA(hm, path, sizeof(path));

            std::string exeFile = path;

            std::string directory;
            const size_t last_slash_idx = exeFile.rfind('\\');
            if (std::string::npos != last_slash_idx)
            {
                    directory = exeFile.substr(0, last_slash_idx);
            }
            return directory;
          #else
                  char buffer[2048];
                  memset(buffer, 0, sizeof(buffer));

                  if (readlink("/proc/self/exe", buffer, sizeof(buffer)) > 0)
                  {
                    std::stringstream ss;
                    ss << buffer;
                    std::string exeFile = ss.str();
                    std::string directory;

                    const size_t last_slash_idx = exeFile.rfind('/');
                    if (std::string::npos != last_slash_idx)
                    {
                            directory = exeFile.substr(0, last_slash_idx);
                    }
                    return directory;
                  }
                  else
                  {
                    return "/";
                  }
          #endif
  }
  
  
  bool set_thread_priority(int64_t thread_id)
  {
    #ifdef __APPLE__
      return false;
        
    #elif _WIN32
    int adjusted_priority = THREAD_PRIORITY_ABOVE_NORMAL;

    int success = 0;
    if (thread_id == 0)
      int success = SetThreadPriority(GetCurrentThread(), adjusted_priority);
    else
    {
      // Not implemented
    }
      
    
    if (success != 0)
    {
      DWORD dwError = GetLastError();
      //std::cerr << "Failed to set thread scheduling " << dwError;
    }
    return success == 0;
    #else
    
    // Normalize the priority for the min/max
    const int POLICY = SCHED_FIFO;
    int priority_min = sched_get_priority_min(POLICY);
    int priority_max = sched_get_priority_max(POLICY);
    float priority_half = ((float) (priority_max + priority_min)) / 2.0 + priority_min;
    int priority_low = priority_half - (priority_half / 2);
    int priority_high = priority_half + (priority_half / 2);
    
    // Valid priority values are -20 to 20
    int corrected_priority = priority_high;
          
    if (thread_id == 0)
    {
      pthread_t thId = pthread_self();
      thread_id = thId;
    }
    
    sched_param sch_params;
    sch_params.sched_priority = corrected_priority;
    
    if(pthread_setschedparam(thread_id, POLICY, &sch_params)) {
      //ALPR_WARN << "Failed to set Thread scheduling : " << errno;
      return false;
    }
    return true;
    
    
    #endif
  }
  
  OPENALPRSUPPORT_DLL_EXPORT bool set_current_thread_high_priority(std::thread &th)
  {
    #ifdef __APPLE__
      return false;
    #else
      return set_thread_priority(th.native_handle());
    #endif
  }
  
  OPENALPRSUPPORT_DLL_EXPORT bool set_current_thread_high_priority()
  {
    return set_thread_priority(0);
  }
  
}

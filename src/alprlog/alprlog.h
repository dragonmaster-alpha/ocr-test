 /*************************************************************************
 * OPENALPR CONFIDENTIAL
 * 
 *  Copyright 2017 OpenALPR Technology, Inc.
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

#ifndef ALPRLOG_H
#define ALPRLOG_H

#include <string>
#include <sstream>

#ifdef _WIN32
  #define OPENALPRLOG_DLL_EXPORT __declspec( dllexport )
#else
  #define OPENALPRLOG_DLL_EXPORT 
#endif

#define ALPR_DEBUG alpr::AlprLogEntry().debug()
#define ALPR_INFO alpr::AlprLogEntry().info()
#define ALPR_WARN alpr::AlprLogEntry().warn()
#define ALPR_ERROR alpr::AlprLogEntry().error()

enum AlprLogLevel {ALPRDEBUG, ALPRINFO, ALPRWARNING, ALPRERROR, ALPRNEVERLOG};

namespace alpr
{

  class AlprLogImpl;
  class OPENALPRLOG_DLL_EXPORT AlprLog {
  public:

    static AlprLog *instance();

    virtual ~AlprLog();
    void setParameters(std::string logger_name, bool write_to_file,
        std::string filename = "", size_t max_file_size_bytes=10 *1024 *1024, int max_backup_files=1);

    void setWriteToString();
    std::string getLogString();
    
    
    void setLogLevel(AlprLogLevel min_level);

    void debug(std::string message);
    void info(std::string message);
    void warn(std::string message);
    void error(std::string message);


  private:
    AlprLog();
    
    bool write_to_string;
    std::stringstream stringbuffer;
    static AlprLog* _instance;

    AlprLogImpl* impl;

  public:



  };

  class AlprLogImpl;
  class OPENALPRLOG_DLL_EXPORT AlprLogEntry
  {
  public:
    AlprLogEntry() {};
    virtual ~AlprLogEntry();
    std::ostringstream& debug();
    std::ostringstream& info();
    std::ostringstream& warn();
    std::ostringstream& error();

  protected:

     std::ostringstream os;
  private:
     AlprLogEntry(const AlprLogEntry&);
     AlprLogEntry& operator =(const AlprLogEntry&);
     AlprLogLevel messageLevel;
  };

}
#endif /* ALPRLOG_H */


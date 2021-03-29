
#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#if defined WINDOWS
#include <dirent.h>

#else
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <errno.h>
#include "exports.h"

namespace alprsupport
{

  struct FileInfo
  {
    int64_t size;
    int64_t creation_time;
  };
  
  OPENALPRSUPPORT_DLL_EXPORT bool startsWith(std::string const &fullString, std::string const &prefix);
  OPENALPRSUPPORT_DLL_EXPORT bool hasEnding (std::string const &fullString, std::string const &ending);
  OPENALPRSUPPORT_DLL_EXPORT bool hasEndingInsensitive(const std::string& fullString, const std::string& ending);

  OPENALPRSUPPORT_DLL_EXPORT std::string filenameWithoutExtension(std::string filename);

  OPENALPRSUPPORT_DLL_EXPORT FileInfo getFileInfo(std::string filename);

  OPENALPRSUPPORT_DLL_EXPORT bool DirectoryExists( const char* pzPath );
  OPENALPRSUPPORT_DLL_EXPORT bool fileExists( const char* pzPath );
  OPENALPRSUPPORT_DLL_EXPORT std::vector<std::string> getFilesInDir(const char* dirPath);

  OPENALPRSUPPORT_DLL_EXPORT bool setOwnerOpenalpr(const char* filepath);
  

  bool makePath(const char* path, mode_t mode);
  OPENALPRSUPPORT_DLL_EXPORT bool stringCompare( const std::string &left, const std::string &right );
    
  OPENALPRSUPPORT_DLL_EXPORT std::string get_directory_from_path(std::string file_path);
  OPENALPRSUPPORT_DLL_EXPORT std::string get_filename_from_path(std::string file_path);
}

#endif // FILESYSTEM_H

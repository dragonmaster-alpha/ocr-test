
#include "string_utils.h"
#include <sstream>
#include <algorithm>
#include <functional>
#include <cctype>

using namespace std;

namespace alprsupport
{

  std::string toString(int value)
  {
    stringstream ss;
    ss << value;
    return ss.str();
  }
  std::string toString(int64_t value)
  {
    stringstream ss;
    ss << value;
    return ss.str();
  }
  std::string toString(uint32_t value)
  {
    return toString((int) value);
  }
  std::string toString(float value)
  {
    stringstream ss;
    ss << value;
    return ss.str();
  }
  std::string toString(double value)
  {
    stringstream ss;
    ss << value;
    return ss.str();
  }

  std::string replaceAll(std::string str, const std::string& from, const std::string& to) {
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos) {
        str.replace(start_pos, from.length(), to);
        start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
    }
    return str;
  }

// trim from start
  std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return s;
  }

// trim from end
  std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
    return s;
  }

// trim from both ends
  std::string &trim(std::string &s) {
    return ltrim(rtrim(s));
  }
  
}

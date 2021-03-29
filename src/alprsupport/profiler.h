#ifndef ALPRSUPPORT_PROFILER_H_
#define ALPRSUPPORT_PROFILER_H_

#include <vector>
#include <string>
#include <memory>
#include <iostream>
#include "exports.h"

#define ALPR_DISABLE_COPY_AND_ASSIGN(classname)            \
private:                                              \
  classname(const classname&) = delete;               \
  classname(classname&&) = delete;                    \
  classname& operator=(const classname&) = delete;    \
  classname& operator=(classname&&) = delete

namespace alprsupport {

/*!
 * \brief Profiler for Caffe, don't enable Profiler in Multi-thread Env
 *  This class is used to profile a range of source code as a scope.
 *  The basic usage is like below.
 *
 * ```
 * Profiler *profiler = Profiler::Get();
 * profiler->ScopeStart("scope1");
 * ...
 * ...
 * profiler->ScopeEnd();
 * ```
 *
 * Scope represents a range of source code. Nested scope is also supported.
 * Dump profile into a json file, then we can view the data from google chrome
 * in chrome://tracing/
 */
#define ALPR_PROF_SCOPE_START(profiler, name) { \
  profiler->ScopeStart(name, __FILE__, __LINE__); \
};

#define ALPR_PROF_SCOPE_END(profiler) { \
  profiler->ScopeEnd(); \
};


class OPENALPRSUPPORT_DLL_EXPORT Profiler {
 public:
  /*! \brief get global instance */
  static Profiler *Get();
  /*!
   * \brief start a scope
   * \param name scope name
   */
  void ScopeStart(const char * name) {
    ScopeStart(name, "(none)", 0);
  }
  /*!
   * \brief start a scope
   * \param name scope name
   * \param file file where this is called from
   * \param line line where this is called from
   */
  void ScopeStart(const char *name, std::string file, int line);

  /*!
   * \brief end a scope
   */
  void ScopeEnd();
  /*!
   * \brief dump profile data
   * \param fn file name
   */
  void DumpProfile(const char *fn) const;
  /*! \brief turn on profiler */
  void TurnON() {
    state_ = kRunning;
  }

  bool isON() const {
    return state_ == kRunning;
  }

  int size() { return scopes_.size(); }

  /*! \brief turn off profiler */
  void TurnOFF() {
    if (state_ != kRunning)
      std::cerr << "Profiler not running" << std::endl;
    if (!scope_stack_.empty())
      std::cerr << "Scope stack is not empty.  Size: " << scope_stack_.size() << std::endl;

    state_ = kNotRunning;
  }
  /*! \brief timestamp, return in microseconds */
  uint64_t Now() const;

  // Used internally
  void _dump_thread_to_file(const char *fn);

 private:
  Profiler();
  ALPR_DISABLE_COPY_AND_ASSIGN(Profiler);

  enum State {
    kRunning,
    kNotRunning,
  };
  struct Scope {
    std::string name;
    uint64_t start_microsec = 0;
    uint64_t end_microsec = 0;
  };
  typedef std::shared_ptr<Scope> ScopePtr;
  /*! \brief scope stack for nested scope */
  std::vector<ScopePtr> scope_stack_;
  /*! \brief all scopes used in profile, not including scopes in stack */
  std::vector<ScopePtr> scopes_;
  /*! \brief init timestamp */
  uint64_t init_;
  /*! \brief profile state */
  State state_;
};  // class Profiler

}  // namespace alprsupport

#endif  // ALPRSUPPORT_PROFILER_H_

#include "profiler.h"

#if defined(_MSC_VER) && _MSC_VER <= 1800
#include <Windows.h>
#else
#include <chrono>
#endif  // _MSC_VER

#include <fstream>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <sstream>
#include <memory>
#include <utility>

namespace alprsupport {

static std::unordered_map<std::thread::id, std::unique_ptr<Profiler>> profiler_by_thread;
static std::mutex init_mutex;

Profiler::Profiler() : init_(Now()), state_(kNotRunning) {
  scope_stack_.reserve(10);
  scopes_.reserve(1024);
}

Profiler *Profiler::Get() {
  const std::lock_guard<std::mutex> lock(init_mutex);
  std::thread::id thread_id = std::this_thread::get_id();
  if (profiler_by_thread.find(thread_id) == profiler_by_thread.end()) {
    std::unique_ptr<Profiler> prof(new Profiler());
    profiler_by_thread.insert(std::make_pair(thread_id, std::move(prof)));
    // profiler_by_thread[thread_id] = prof;
  }
  return profiler_by_thread[thread_id].get();
}

void Profiler::ScopeStart(const char *name, std::string file, int line) {
  if (state_ == kNotRunning) return;
  ScopePtr scope(new Scope);
  if (!scope_stack_.empty()) {
    scope->name = scope_stack_.back()->name + ":" + name;
  } else {
    scope->name = name;
  }
  if (line > 0)
    scope->name = scope->name + "(" + file + ":" + std::to_string(line) + ")";
  scope->start_microsec = Now() - init_;
  scope_stack_.push_back(scope);
}

void Profiler::ScopeEnd() {
  if (state_ == kNotRunning)
    return;
  ScopePtr current_scope = scope_stack_.back();
  current_scope->end_microsec = Now() - init_;
  scopes_.push_back(current_scope);
  // pop stack
  scope_stack_.pop_back();
}

uint64_t Profiler::Now() const {
#if defined(_MSC_VER) && _MSC_VER <= 1800
  LARGE_INTEGER frequency, counter;
  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&counter);
  return counter.QuadPart * 1000000 / frequency.QuadPart;
#else
  return std::chrono::duration_cast<std::chrono::microseconds>(
      std::chrono::high_resolution_clock::now().time_since_epoch()).count();
#endif  // _MSC_VER
}

static void ProfilerWriteEvent(std::ofstream &file, const char *name, const char *ph, uint64_t ts) {
  file << "    {" << std::endl;
  file << "      \"name\": \"" << name << "\"," << std::endl;
  file << "      \"cat\": \"category\"," << std::endl;
  file << "      \"ph\": \"" << ph << "\"," << std::endl;
  file << "      \"ts\": " << ts << "," << std::endl;
  file << "      \"pid\": 0," << std::endl;
  file << "      \"tid\": 0" << std::endl;
  file << "    }";
}

void Profiler::_dump_thread_to_file(const char *fn) {
  std::ofstream file;
  file.open(fn);
  file << "{" << std::endl;
  file << "  \"traceEvents\": [";

  bool is_first = true;
  for (auto scope : this->scopes_) {
    if (is_first) {
      file << std::endl;
      is_first = false;
    } else {
      file << "," << std::endl;
    }
    ProfilerWriteEvent(file, scope->name.c_str(), "B", scope->start_microsec);
    file << "," << std::endl;
    ProfilerWriteEvent(file, scope->name.c_str(), "E", scope->end_microsec);
  }

  file << "  ]," << std::endl;
  file << "  \"displayTimeUnit\": \"ms\"" << std::endl;
  file << "}" << std::endl;
}
// Assume no further activity after dumping
void Profiler::DumpProfile(const char *fn) const {
  if (!scope_stack_.empty()) {
    std::cerr << "Stack not empty" << std::endl;
    for (auto & x : scope_stack_)
      std::cout << "x.name " << x->name << std::endl;
    return;
  }

  if (!scope_stack_.empty() && (state_ != kNotRunning)) {
    std::cerr << "Profiler not running, can't dump profile" << std::endl;
    return;
  }

  int enabled_count = 0;
  for (auto & x : profiler_by_thread) {
    if (x.second->size() > 0)
      enabled_count++;
  }

  for (auto & x : profiler_by_thread) {
    if (x.second->size() == 0)
      continue;

    std::stringstream ss;
    if (enabled_count > 1)
      ss << fn << ".thread_" << x.first;
    else
      ss << fn;

    x.second->_dump_thread_to_file(ss.str().c_str());
  }
}
}  // namespace alprsupport

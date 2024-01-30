/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Metrics.h"

#include <cstring>
#include <filesystem>
#include <sys/resource.h>
#include <unistd.h>

std::size_t Metrics::_pid;
std::string Metrics::_procFdPath;
std::string Metrics::_procThreadPath;
std::size_t Metrics::_maxRss = 0;
int Metrics::_numberThreads = 0;
int Metrics::_numberOpenFDs = 0;

void Metrics::save() {
  _pid = getpid();
  std::string prefix = std::string("/proc/").append(std::to_string(_pid));
  _procFdPath = prefix;
  _procFdPath.append("/fd");
  _procThreadPath = prefix;
  _procThreadPath.append("/task");
  try {
    _maxRss = getMaxRss();
    _numberThreads =
      std::distance(std::filesystem::directory_iterator(_procThreadPath), std::filesystem::directory_iterator{});
    _numberOpenFDs =
      std::distance(std::filesystem::directory_iterator(_procFdPath), std::filesystem::directory_iterator{});
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
}

// /proc/36258/status | grep Threads
void Metrics::print(LOG_LEVEL level,
		    std::ostream& stream,
		    bool displayLevel) {
  Logger logger(level, stream, displayLevel);
  logger << "\tmaxRss=" << _maxRss << '\n'
    << "\tnumberThreads=" << _numberThreads << '\n'
    << "\t\'lsof\'=" << _numberOpenFDs << '\n';
}

std::size_t Metrics::getMaxRss() {
  struct rusage usage;
  if (getrusage(RUSAGE_SELF, &usage)) {
    LogError << strerror(errno) << '\n';
    return 0;
  }
  return usage.ru_maxrss;
}

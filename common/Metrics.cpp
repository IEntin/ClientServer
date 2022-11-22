/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Metrics.h"
#include "Utility.h"
#include <filesystem>
#include <sys/resource.h>

Metrics::Metrics() : _pid(getpid()) {
  std::string prefix = std::string("/proc/").append(std::to_string(_pid));
  _procFdPath = prefix;
  _procFdPath.append("/fd");
  _procThreadPath = prefix;
  _procThreadPath.append("/task");
}
// /proc/36258/status | grep Threads
void Metrics::print() {
  try {
    size_t numberFds =
      std::distance(std::filesystem::directory_iterator(_procFdPath), std::filesystem::directory_iterator{});
    CERR << "\t\'lsof\'=" << numberFds << std::endl;
    size_t numberThreads =
      std::distance(std::filesystem::directory_iterator(_procThreadPath), std::filesystem::directory_iterator{});
    CERR << "\tnumberThreads=" << numberThreads << std::endl;
    CERR << "\tmaxRss=" << getMaxRss() << std::endl;
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
}

size_t Metrics::getMaxRss() {
  try {
    struct rusage usage;
    if (getrusage(RUSAGE_SELF, &usage)) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << strerror(errno) << std::endl;
      return 0;
    }
    return usage.ru_maxrss;
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' ' << e.what() << std::endl;
  }
  return 0;
}

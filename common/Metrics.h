/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Logger.h"

class Metrics {
 public:
  Metrics() = delete;
  ~Metrics() = delete;
  static size_t getMaxRss();
  static void save();
  static void print(LOG_LEVEL level = LOG_LEVEL::INFO,
		    std::ostream& stream = std::clog,
		    bool displayLevel = true);
 private:
  static size_t _pid;
  static std::string _procFdPath;
  static std::string _procThreadPath;
  static size_t _maxRss;
  static int _numberThreads;
  static int _numberOpenFDs;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cstddef>
#include <string>

class Metrics {
 public:
  Metrics() = delete;
  ~Metrics() = delete;
  static size_t getMaxRss();
  static void save();
  static void print();
 private:
  static size_t _pid;
  static std::string _procFdPath;
  static std::string _procThreadPath;
  static size_t _maxRss;
  static unsigned _numberThreads;
  static unsigned _numberOpenFDs;
};

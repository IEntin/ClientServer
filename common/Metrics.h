/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cstddef>
#include <string>

class Metrics {
 public:
  Metrics();
  ~Metrics() = default;
  size_t getMaxRss();
  void print();
 private:
  const size_t _pid;
  std::string _procFdPath;
  std::string _procThreadPath;
};

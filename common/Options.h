/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

struct Options {
  Options() = delete;
  ~Options() = delete;
  static void parse(class AppOptions& appOptions);
  static std::string _fifoDirectoryName;
  static std::string _acceptorName;
};

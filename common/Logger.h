/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

enum class LOG_LEVEL : char {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR
};

inline constexpr std::string_view levelNames[] {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR"
};

using Error = struct Logger;

struct Logger {
  Logger(LOG_LEVEL level, std::ostream& stream = std::clog, bool displayLevel = true) :
    _level(level),
    _stream(_level >= _threshold ? stream : _nullStream),
    _displayLevel(displayLevel) {
    printLevel();
  }
  Logger(bool displayLevel = true) :
    _level(LOG_LEVEL::ERROR),
    _stream(std::cerr),
    _displayLevel(displayLevel) {
    printLevel();
  }
  void printLevel() {
    if (_displayLevel)
      _stream << '[' << levelNames[static_cast<int>(_level)] << ']';
  }
  ~Logger() {}
  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayLevel;
  auto& operator <<(std::string_view value) {
    return _stream << value;
  }
  static LOG_LEVEL _threshold;
  static std::ofstream _nullStream;
};

inline LOG_LEVEL translateLogThreshold(std::string_view configName) {
  for (unsigned index = 0; index < sizeof(levelNames)/sizeof(std::string_view); ++index) {
    if (configName == levelNames[index]) {
      Logger::_threshold = static_cast<LOG_LEVEL>(index);
      return Logger::_threshold;
    }
  }
  return LOG_LEVEL::TRACE;
}
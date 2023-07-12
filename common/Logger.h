/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

#define CODELOCATION __FILE__ << ':' << __LINE__ << ' ' << __func__

#define LogError Logger(LOG_LEVEL::ERROR, std::cerr) << CODELOCATION << ':'
#define Warn Logger(LOG_LEVEL::WARN, std::clog) << CODELOCATION << ':'
#define Info Logger(LOG_LEVEL::INFO, std::clog) << CODELOCATION << ':'
#define Debug Logger(LOG_LEVEL::DEBUG, std::clog) << CODELOCATION << ':'
#define Trace Logger(LOG_LEVEL::TRACE, std::clog) << CODELOCATION << ':'

enum class LOG_LEVEL : char {
  ALL,
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR
};

inline constexpr std::string_view levelNames[] {
  "ALL",
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR"
};

struct Logger {
  explicit Logger(LOG_LEVEL level, std::ostream& stream = std::clog, bool displayLevel = true) :
    _level(level),
    _stream(_level >= _threshold ? stream : _nullStream),
    _displayLevel(displayLevel) {
    printLevel();
  }
  explicit Logger(bool displayLevel = true) :
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
  template <typename V>
  auto& operator <<(const V& value) {
    return _stream << value;
  }
  std::osyncstream& getStream() { return _stream; }
  static inline LOG_LEVEL _threshold = LOG_LEVEL::ERROR;
  static inline std::ofstream _nullStream = std::ofstream("");
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

#define CODELOCATION __FILE__, __LINE__, __func__

#define LogError Logger(LOG_LEVEL::ERROR, std::cerr).printPrefix(CODELOCATION)
#define Warn Logger(LOG_LEVEL::WARN, std::clog).printPrefix(CODELOCATION)
#define Info Logger(LOG_LEVEL::INFO, std::clog).printPrefix(CODELOCATION)
#define Debug Logger(LOG_LEVEL::DEBUG, std::clog).printPrefix(CODELOCATION)
#define Trace Logger(LOG_LEVEL::TRACE, std::clog).printPrefix(CODELOCATION)

enum class LOG_LEVEL : char {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  ALWAYS = ERROR
};

inline constexpr std::string_view levelNames[] {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "ALWAYS"
};

struct Logger {
  explicit Logger(LOG_LEVEL level, std::ostream& stream = std::clog, bool displayPrefix = true) :
    _level(level),
    _stream(stream),
    _displayPrefix(displayPrefix) {
  }
  ~Logger() {}

  std::ostream& printPrefix(const char* file, int line, const char* func) {
    try {
      if (_level < _threshold) {
 	_stream.setstate(std::ios_base::failbit);
	return _stream;
      }
      if (!_displayPrefix)
	return _stream;
      return _stream << '[' << levelNames[static_cast<int>(_level)] << ']'
		     << file << ':' << line << ' ' << func << ':';
    }
    catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
    return _stream;
  }

  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayPrefix;
  template <typename V>
  std::ostream& operator <<(const V& value) {
    try {
      if (_level >= _threshold)
	return _stream << value;
    }
    catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
    return _stream;
  }
  std::osyncstream& getStream() { return _stream; }

  static inline LOG_LEVEL _threshold = LOG_LEVEL::ERROR;
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

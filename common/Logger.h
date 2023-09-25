/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/core/noncopyable.hpp>
#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

#define CODELOCATION __FILE__, __LINE__, __func__

#define LogAlways Logger(LOG_LEVEL::ALWAYS, std::cerr).printPrefix(CODELOCATION)
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
  ALWAYS
};

constexpr std::string_view levelNames[] {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "ERROR",
  "ALWAYS"
};

struct Logger : private boost::noncopyable {
  explicit Logger(LOG_LEVEL level, std::ostream& stream = std::clog, bool displayPrefix = true) :
    _level(level),
    _stream(stream),
    _displayPrefix(displayPrefix) {
  }
  
  ~Logger() {
    _stream.flush();
  }

  Logger& printPrefix(const char* file, int line, const char* func) {
    try {
      if (_level < _threshold || !_displayPrefix)
	return *this;
      _stream << '[' << levelNames[static_cast<int>(_level)] << ']'
	      << file << ':' << line << ' ' << func << ':';
      return *this;
    }
    catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
    return *this;
  }

  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayPrefix;

  template <typename V>
  Logger& operator <<(const V& value) {
    try {
      if (_level >= _threshold) {
	_stream << value;
	return *this;
      }
    }
    catch (const std::exception& e) {
      std::cerr << e.what() << std::endl;
    }
    return *this;
  }

  std::osyncstream& getStream() { return _stream; }

  static void translateLogThreshold(std::string_view configName) {
    for (unsigned index = 0; index < sizeof(levelNames)/sizeof(std::string_view); ++index) {
      if (configName == levelNames[index]) {
	_threshold = static_cast<LOG_LEVEL>(index);
      }
    }
  }

  static inline LOG_LEVEL _threshold = LOG_LEVEL::ALWAYS;
};

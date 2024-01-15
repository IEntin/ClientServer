/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

#include <boost/core/noncopyable.hpp>

#include "IOUtility.h"

#define CODELOCATION __FILE__, __LINE__, __func__

#define LogAlways Logger(LOG_LEVEL::ALWAYS, std::clog).printPrefix(CODELOCATION)
#define LogError Logger(LOG_LEVEL::ERROR, std::cerr).printPrefix(CODELOCATION)
#define Warn Logger(LOG_LEVEL::WARN, std::cerr).printPrefix(CODELOCATION)
#define Info Logger(LOG_LEVEL::INFO, std::clog).printPrefix(CODELOCATION)
#define Debug Logger(LOG_LEVEL::DEBUG, std::clog).printPrefix(CODELOCATION)
#define Trace Logger(LOG_LEVEL::TRACE, std::clog).printPrefix(CODELOCATION)

enum class LOG_LEVEL : int {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  ERROR,
  ALWAYS,
  NUMBEROF
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
    _stream << std::boolalpha;
  }
  
  ~Logger() {
    _stream.flush();
  }

  Logger& printPrefix(const char* file, int line, const char* func);

  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayPrefix;

  std::osyncstream& getStream() { return _stream; }

  static void translateLogThreshold(std::string_view configName);

  static LOG_LEVEL _threshold;
};

void integerWrite(Logger& logger, long value);

template<ioutility::Integer N>
Logger& operator <<(Logger& logger, const N& value) {
  if (logger._level >= Logger::_threshold) {
    integerWrite(logger, value);
  }
  return logger;
}

template <typename V>
Logger& operator <<(Logger& logger, const V& value) {
  try {
    if (logger._level >= Logger::_threshold) {
      logger._stream << value;
      return logger;
    }
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return logger;
}

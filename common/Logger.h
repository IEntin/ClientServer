/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>

#include <boost/assert/source_location.hpp>
#include <boost/core/noncopyable.hpp>
#include <boost/core/noncopyable.hpp>

#include "IOUtility.h"

#define LogAlways Logger(LOG_LEVEL::ALWAYS, std::clog).printPrefix()
#define LogError Logger(LOG_LEVEL::ERROR, std::cerr).printPrefix()
#define Expected Logger(LOG_LEVEL::EXPECTED, std::cerr).printPrefix()
#define Warn Logger(LOG_LEVEL::WARN, std::cerr).printPrefix()
#define Info Logger(LOG_LEVEL::INFO, std::clog).printPrefix()
#define Debug Logger(LOG_LEVEL::DEBUG, std::clog).printPrefix()
#define Trace Logger(LOG_LEVEL::TRACE, std::clog).printPrefix()

enum class LOG_LEVEL : int {
  TRACE,
  DEBUG,
  INFO,
  WARN,
  EXPECTED,
  ERROR,
  ALWAYS,
  NUMBEROF
};

constexpr std::string_view levelNames[] {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "EXPECTED",
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

  Logger& printPrefix(const boost::source_location& location = BOOST_CURRENT_LOCATION);

  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayPrefix;

  std::osyncstream& getStream() { return _stream; }

  static void translateLogThreshold(std::string_view configName);

  static LOG_LEVEL _threshold;

  static void integerWrite(Logger& logger, long value);
};

template<ioutility::Integer N>
Logger& operator <<(Logger& logger, const N& value) {
  if (logger._level >= Logger::_threshold) {
    Logger::integerWrite(logger, value);
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

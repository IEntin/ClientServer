/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iomanip>
#include <iostream>
#include <string_view>
#include <syncstream>
#include <utility>

#include <boost/assert/source_location.hpp>
#include <boost/core/noncopyable.hpp>

#define LogAlways Logger(LOG_LEVEL::ALWAYS, std::clog).printPrefix()
#define LogError Logger(LOG_LEVEL::ERROR, std::cerr).printPrefix()
#define Expected Logger(LOG_LEVEL::EXPECTED, std::clog).printPrefix()
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
  INVALID
};

constexpr std::string_view levelNames[] {
  "TRACE",
  "DEBUG",
  "INFO",
  "WARN",
  "EXPECTED",
  "ERROR",
  "ALWAYS",
  "INVALID"
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
};

template <typename V>
Logger& operator <<(Logger& logger, const V& value) {
  try {
    if (logger._level >= Logger::_threshold) {
      if constexpr (std::is_scoped_enum_v<V>) {
	logger._stream << std::to_underlying(value);
	return logger;
      }
      else {
	logger._stream << value;
	return logger;
      }
    }
  }
  catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
  return logger;
}

template <std::size_t INDEX = 0, typename TUPLE>
constexpr void printTuple(const TUPLE& tuple, Logger& logger) {
  constexpr std::size_t last = std::tuple_size_v<TUPLE> - 1;
  if constexpr (INDEX < last) {
    logger << std::get<INDEX>(tuple) << ',';
    printTuple<INDEX + 1>(tuple, logger);
  }
  else if constexpr (INDEX == last) {
    logger << std::get<INDEX>(tuple) << '\n';
    return;
  }
  else
    throw std::runtime_error("Out of bounds");
}

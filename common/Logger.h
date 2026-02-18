/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <iostream>
#include <syncstream>

#include <boost/algorithm/hex.hpp>
#include <boost/assert/source_location.hpp>
#include <boost/core/noncopyable.hpp>

#define LogAlways (Logger(LOG_LEVEL::ALWAYS).printLocation(BOOST_CURRENT_LOCATION))
#define LogError (Logger(LOG_LEVEL::ERROR, std::cerr).printLocation(BOOST_CURRENT_LOCATION))
#define Expected (Logger(LOG_LEVEL::EXPECTED).printLocation(BOOST_CURRENT_LOCATION))
#define Warn (Logger(LOG_LEVEL::WARN, std::cerr).printLocation(BOOST_CURRENT_LOCATION))
#define Info (Logger(LOG_LEVEL::INFO).printLocation(BOOST_CURRENT_LOCATION))
#define Debug (Logger(LOG_LEVEL::DEBUG).printLocation(BOOST_CURRENT_LOCATION))
#define Trace (Logger(LOG_LEVEL::TRACE).printLocation(BOOST_CURRENT_LOCATION))

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
  explicit Logger(LOG_LEVEL level, std::ostream& stream = std::clog, bool displayLocation = true) :
    _level(level),
    _stream(stream),
    _displayLocation(displayLocation) {
    _stream << std::boolalpha;
  }
  
  ~Logger() {
    _stream.flush();
  }

  Logger& printLocation(const boost::source_location& location);

  const LOG_LEVEL _level;
  std::osyncstream _stream;
  const bool _displayLocation;

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

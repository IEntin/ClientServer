/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <chrono>

#include <boost/assert/source_location.hpp>

#include "Logger.h"

struct Chronometer : private boost::noncopyable {
  explicit Chronometer(bool enable = true,
		       std::ostream* pstream = nullptr,
		       const boost::source_location& location = BOOST_CURRENT_LOCATION) :
    _enabled(enable), _file(location.file_name()), _line(location.line()), _function(location.function_name()),
    _stream(pstream ? *pstream : std::clog) {
    if (_enabled) {
      _globalStart = std::chrono::steady_clock::now();
      _localStart = _globalStart;
    }
  }

  // usage: chronometer.start();
  // where chronometer is an object created somewhere.
  void start(const boost::source_location& location = BOOST_CURRENT_LOCATION) {
    if (_enabled) {
      _localStart = std::chrono::steady_clock::now();
      Logger logger(LOG_LEVEL::INFO, _stream);
      logger << __func__ << '-' << location.file_name() << ':'
	     << location.line() << ' ' << location.function_name() << '\n';
    }
  }

  // usage: chronometer.stop();
  // shows elapsed time since start or construction.
  void stop(const boost::source_location& location = BOOST_CURRENT_LOCATION) {
    if (_enabled) {
      auto end{ std::chrono::steady_clock::now() };
      std::chrono::duration<double> elapsed_seconds{ end - _localStart };
      Logger logger(LOG_LEVEL::INFO, _stream);
      logger << __func__ << '-' << location.file_name() << ':' << location.line() << ' '
	     << location.function_name() << std::fixed << std::setprecision(3) << " elapsed="
	     << elapsed_seconds.count() << 's' << '\n';
    }
  }
  // shows elapsed time since construction.
  ~Chronometer() {
    if (_enabled) {
      auto end{ std::chrono::steady_clock::now() };
      std::chrono::duration<double> elapsed_seconds{ end - _globalStart };
      Logger logger(LOG_LEVEL::INFO, _stream);
      logger << __func__ << ':' << _file << ':' << _line << ' '
	<< _function << ' ' << std::fixed << std::setprecision(3) << "elapsed="
	<< elapsed_seconds.count() << 's' << '\n';
    }
  }
 private:
  const bool _enabled;
  const std::string _file;
  const int _line;
  const std::string _function;
  std::osyncstream _stream;
  std::chrono::time_point<std::chrono::steady_clock> _globalStart;
  std::chrono::time_point<std::chrono::steady_clock> _localStart;
};

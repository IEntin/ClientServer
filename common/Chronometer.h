/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <chrono>
#include <iomanip>
#include <iostream>

struct Chronometer {
  explicit Chronometer(bool enable = true,
		       std::string_view file = __FILE__,
		       int line = __LINE__,
		       std::string_view function = "global",
		       std::ostream* pstream = nullptr) :
    _enabled(enable), _file(file), _line(line), _function(function),
    _stream(pstream ? *pstream : std::clog) {
    if (_enabled) {
      _globalStart = std::chrono::steady_clock::now();
      _localStart = _globalStart;
    }
  }

  void start(const char* file, const char* function, int line) {
    if (_enabled) {
      _localStart = std::chrono::steady_clock::now();
      _stream << __func__ << '-' << file << ':' << line << ' ' << function << std::endl;
    }
  }

  void stop(const char* file, const char* function, int line) {
    if (_enabled) {
      auto end{ std::chrono::steady_clock::now() };
      std::chrono::duration<double> elapsed_seconds{ end - _localStart };
      _stream << __func__ << '-' << file << ':' << line << ' ' << _function
	      << std::fixed << std::setprecision(3) << " elapsed="
	      << elapsed_seconds.count() << 's' << std::endl;
    }
  }

  ~Chronometer() {
    if (_enabled) {
      auto end{ std::chrono::steady_clock::now() };
      std::chrono::duration<double> elapsed_seconds{ end - _globalStart };
      _stream << __func__ << ':' << _file << ':' << _line << ' ' << _function
	      << ' ' << std::fixed << std::setprecision(3) << "elapsed="
	      << elapsed_seconds.count() << 's' << std::endl;
    }
  }

  Chronometer(const Chronometer&) = delete;
  Chronometer& operator=(const Chronometer&) = delete;
  const bool _enabled;
  const std::string _file;
  const int _line;
  const std::string _function;
  std::ostream& _stream;
  std::chrono::time_point<std::chrono::steady_clock> _globalStart;
  std::chrono::time_point<std::chrono::steady_clock> _localStart;
};

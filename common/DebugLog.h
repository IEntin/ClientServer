/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <ranges>
#include <syncstream>

#include <boost/algorithm/hex.hpp>

#include "Options.h"

enum class APPTYPE : int {
  TESTS,
  SERVER,
  CLIENT
};

class DebugLog {

  static std::ofstream _file;
  static std::mutex _mutex;
  DebugLog() = delete;
  ~DebugLog() = delete;
 public:
  static void setDebugLog(APPTYPE type);

  template <typename T>
  static bool logBinaryData([[maybe_unused]] const boost::source_location& loc,
			    [[maybe_unused]] std::string_view name,
			    [[maybe_unused]] const T& variable) {
    if constexpr (Options::_debug) {
      static std::osyncstream stream(_file);
      std::lock_guard lock(_mutex);
      stream << '\n' << loc.file_name() << ':' << loc.line() << '-' << loc.function_name() << '\n';
      stream << name << ",size=" << std::ranges::ssize(variable) << "\n0x";
      boost::algorithm::hex(std::cbegin(variable), std::cend(variable), std::ostream_iterator<char> { stream });
      stream << '\n';
      stream.flush();
    }
    // to be able to print just once
    return true;
  }

  static void setTitle([[maybe_unused]] std::string_view title) {
    if constexpr (Options::_debug)
      _file << "\n**\n" << title << "\n**";
  }

};

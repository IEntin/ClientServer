/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <boost/algorithm/hex.hpp>

class DebugLog {

  static std::ofstream _file;

  DebugLog() = delete;
  ~DebugLog() = delete;
 public:

  template <typename T>
  static void logBinaryData([[maybe_unused]] const boost::source_location& loc,
			    [[maybe_unused]] std::string_view name,
			    [[maybe_unused]] const T& variable) {
#ifdef _DEBUG
    _file << '\n' << loc.file_name() << ':' << loc.line() << '-' << loc.function_name() << '\n';
    _file << name << " 0x";
    boost::algorithm::hex(variable, std::ostream_iterator<char> { _file });
    _file << '\n';
#endif
}

#ifdef _DEBUG
  static void setTitle(std::string_view title) {
    _file << "\n**\n" << title << "\n**";
  }
#endif

};

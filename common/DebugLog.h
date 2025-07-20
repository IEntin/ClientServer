/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fstream>
#include <boost/algorithm/hex.hpp>

enum class APPTYPE : int {
  TESTS,
  SERVER,
  CLIENT
};

class DebugLog {

  static std::ofstream _file;

  DebugLog() = delete;
  ~DebugLog() = delete;
 public:
  static void setDebugLog([[maybe_unused]] APPTYPE type);

  template <typename T>
  static void logBinaryData([[maybe_unused]] const boost::source_location& loc,
			    [[maybe_unused]] std::string_view name,
			    [[maybe_unused]] const T& variable) {
#ifdef _DEBUG
    _file << '\n' << loc.file_name() << ':' << loc.line() << '-' << loc.function_name() << '\n';
    _file << name << " 0x";
    boost::algorithm::hex(variable, std::ostream_iterator<char> { _file });
    _file << '\n';
    _file.flush();
#endif
}

  static void setTitle([[maybe_unused]] std::string_view title) {
#ifdef _DEBUG
    _file << "\n**\n" << title << "\n**";
#endif
  }

};

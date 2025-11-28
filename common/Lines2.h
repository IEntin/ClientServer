/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <boost/static_string/static_string.hpp>
#include <boost/core/noncopyable.hpp>

#include "Logger.h"
#include "Utility.h"

class Lines2 : private boost::noncopyable {
 public:
  // The line can be a string_view or a string depending on
  // the usage. If the line is used up by the app before the
  // next line is created use a string_view, it is
  // backed up by the buffer. Otherwise use a string.
  long _index = -1;
  long _maxIndex;
  bool _last = false;
  std::deque<std::string_view> _lines;
  explicit Lines2(char delimiter = '\n', bool keepDelimiter = false);
  virtual ~Lines2() = default;
  virtual bool getLine(std::string&) = 0;
 protected:
  static constexpr long MAXSUBSTRSIZE = 2000;
  boost::static_strings::static_string<MAXSUBSTRSIZE> _substr;
  const char _delimiter;
  const bool _keepDelimiter;
  std::size_t _inputSize = 0;
};

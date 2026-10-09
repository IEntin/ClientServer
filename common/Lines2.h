/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <deque>
#include <string_view>
#include <vector>

#include <boost/core/noncopyable.hpp>

class Lines2 : private boost::noncopyable {
 public:
  long _index = -1;
  static thread_local std::vector<std::string_view> _lines;
  explicit Lines2(char delimiter = '\n', bool keepDelimiter = false);
  virtual ~Lines2() = default;
  virtual bool getLine(std::string_view&) = 0;
 protected:
  const char _delimiter;
  const bool _keepDelimiter;
};

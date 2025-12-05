/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines2.h"

thread_local std::deque<std::string> Lines2::_lines;

Lines2::Lines2(char delimiter, bool keepDelimiter) :
  _delimiter(delimiter), _keepDelimiter(keepDelimiter) {}

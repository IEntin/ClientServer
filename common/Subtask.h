/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <boost/core/noncopyable.hpp>
#include <atomic>

struct Subtask : private boost::noncopyable {
  Subtask() = default;
  ~Subtask() = default;
  HEADER _header;
  std::string _body;
  std::atomic<STATUS> _state = STATUS::NONE;
};

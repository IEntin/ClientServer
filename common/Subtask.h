/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <atomic>

struct Subtask {
  Subtask() = default;
  Subtask(const Subtask&) = delete;
  ~Subtask() = default;
  Subtask& operator=(Subtask& other) = delete;
  HEADER _header;
  std::string _body;
  std::atomic<STATUS> _state = STATUS::NONE;
};

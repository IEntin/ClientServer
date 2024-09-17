/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

#include "Header.h"

struct Subtask {
  Subtask() = default;
  ~Subtask() = default;
  std::string _data;
  std::atomic<STATUS> _state = STATUS::NONE;
};

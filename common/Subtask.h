/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

struct Subtask {
  Subtask() = default;
  ~Subtask() = default;
  std::string _body;
  std::atomic<STATUS> _state = STATUS::NONE;
};

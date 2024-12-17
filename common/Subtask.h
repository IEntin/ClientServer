/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>
#include <deque>

#include "Header.h"

using Subtasks = std::deque<struct Subtask>;

struct Subtask {
  Subtask() {}
  ~Subtask() {}
  std::string _data;
  std::atomic<STATUS> _state = STATUS::NONE;

  static void clearTask(Subtasks& task) {
    for (auto& subtask : task) {
      subtask._data.erase(subtask._data.cbegin(), subtask._data.cend());
      subtask._state = STATUS::NONE;
    }
  }
};

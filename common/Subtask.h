/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"

using Subtasks = std::vector<struct Subtask>;

struct Subtask {
  Subtask() {}
  ~Subtask() {}
  std::string _data;
  STATUS _state = STATUS::NONE;
  HEADER _header;

  static void clearTask(Subtasks& task) {
    for (auto& subtask : task) {
      subtask._data.erase(subtask._data.cbegin(), subtask._data.cend());
      subtask._state = STATUS::NONE;
    }
  }
};

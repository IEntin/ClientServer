/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"

using Subtasks = std::vector<struct Subtask>;

struct Subtask {
  Subtask() = default;
  ~Subtask() = default;
  std::string _data;
  STATUS _state = STATUS::NONE;
  HEADER _header;
};

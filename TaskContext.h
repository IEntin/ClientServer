#pragma once

#include "Utility.h"

struct TaskContext {
  TaskContext() = default;
  TaskContext(std::string_view headerView, HEADER& header);
  TaskContext(const HEADER& header);
  ~TaskContext() = default;

  HEADER _header;
  bool _diagnostics = false;
};

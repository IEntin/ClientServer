/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

struct Globals {
  static inline std::atomic_flag _stopFlag;
};

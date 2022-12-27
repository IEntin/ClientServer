/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <atomic>

struct Globals {
  static std::atomic_flag _stopFlag;
};

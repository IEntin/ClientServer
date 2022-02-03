#pragma once
#include <atomic>

struct Diagnostics {
  static std::atomic<bool> _enabled;
  static bool enabled() { return _enabled; }
  static void enable(bool enabled) { _enabled = enabled; }
};

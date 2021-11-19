#pragma once

#include "ProgramOptions.h"
#include <vector>

struct MemoryPool {
  static thread_local std::vector<char> _buffer;
  static thread_local std::vector<char> _receiveBuffer;
  static std::pair<char*, size_t> getBuffer();
  static std::pair<char*, size_t> getBuffer(size_t capacity);
  static std::pair<char*, size_t> getReceiveBuffer(size_t capacity);
private:
  MemoryPool() = delete;
  ~MemoryPool() = delete;
};

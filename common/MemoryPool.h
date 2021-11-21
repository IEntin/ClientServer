#pragma once

#include <vector>

struct MemoryPool {
  static thread_local std::vector<char> _primaryBuffer;
  static thread_local std::vector<char> _secondaryBuffer;
  static std::pair<char*, size_t> getPrimaryBuffer(size_t capacity);
  static std::vector<char>& getSecondaryBuffer(size_t capacity);
private:
  MemoryPool() = delete;
  ~MemoryPool() = delete;
};

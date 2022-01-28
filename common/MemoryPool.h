/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  static size_t _initialBufferSize;
  static thread_local std::vector<char> _primaryBuffer;
  static thread_local std::vector<char> _secondaryBuffer;
  static std::pair<char*, size_t> getPrimaryBuffer(size_t capacity);
  static std::vector<char>& getSecondaryBuffer(size_t capacity);
  static size_t getInitialBufferSize();
  static void setup(size_t initialSize);
private:
  MemoryPool() = delete;
  ~MemoryPool() = delete;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool();
  ~MemoryPool() = default;
  static void setup(size_t initialBufferSize);
  std::vector<char> _primaryBuffer;
  std::vector<char> _secondaryBuffer;
  static std::pair<char*, size_t> getPrimaryBuffer(size_t capacity);
  static std::vector<char>& getSecondaryBuffer(size_t capacity);
  static size_t getInitialBufferSize();
private:
static MemoryPool& instance();
  static size_t _initialBufferSize;
};

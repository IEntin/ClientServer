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
  static std::vector<char>& getPrimaryBuffer(size_t capacity = 0);
  static std::vector<char>& getSecondaryBuffer(size_t capacity = 0);
  private:
  static MemoryPool& instance();
  static size_t _initialBufferSize;
};

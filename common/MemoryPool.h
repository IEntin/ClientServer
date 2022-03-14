/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool();
  ~MemoryPool();
  static void setup(size_t initialBufferSize);
  std::vector<char> _primaryBuffer;
  std::vector<char> _secondaryBuffer;
  size_t _perThreadBufferSize = 0;
  static std::vector<char>& getPrimaryBuffer(size_t capacity = 0);
  static std::vector<char>& getSecondaryBuffer(size_t capacity = 0);
  static size_t getInitialBufferSize() { return _initialBufferSize; }
  private:
  void resetBufferSize();
  static MemoryPool& instance();
  static size_t _initialBufferSize;
};

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool() = default;
  ~MemoryPool() = default;
  void setSuggestedSize(size_t suggestedSize) { _suggestedSize = suggestedSize; }
  std::vector<char> _buffer;
  std::vector<char>& getBuffer(size_t capacity = 0);
  size_t getSuggestedSize() const { return _suggestedSize; }
  static void destroyBuffer();
 private:
  static MemoryPool& instance();
  // _suggestedSize must be static,
  // otherwise it will be set only in
  // one and often irrelevant thread.
  // Instances of MemoryPool struct are
  // thread_local, see implementation.
  static size_t _suggestedSize;
};

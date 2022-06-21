/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool() = default;
  ~MemoryPool() = default;
  void setInitialSize(size_t initialBufferSize);
  std::vector<char> _buffer;
  std::vector<char>& getBuffer(size_t capacity = 0);
  size_t getInitialBufferSize() const { return _initialBufferSize; }
  static void destroyBuffer();
 private:
  static MemoryPool& instance();
  // _initialBufferSize must be static,
  // otherwise it will be set only in
  // one and often irrelevant thread.
  // Instances of MemoryPool struct are
  // thread_local, see implementation.
  static size_t _initialBufferSize;
};

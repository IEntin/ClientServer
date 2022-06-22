/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool() = default;
  ~MemoryPool() = default;
  void setExpectedSize(size_t expectedSize) { _expectedSize = expectedSize; }
  std::vector<char> _buffer;
  std::vector<char>& getBuffer(size_t capacity = 0);
  size_t getExpectedSize() const { return _expectedSize; }
  static void destroyBuffer();
 private:
  static MemoryPool& instance();
  // _expectedSize must be a static member,
  // otherwise it will be set only in
  // one and often irrelevant thread.
  // Instances of MemoryPool struct are
  // thread_local.
  static size_t _expectedSize;
};

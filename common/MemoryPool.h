/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <vector>

struct MemoryPool {
  MemoryPool() = default;
  ~MemoryPool() = default;
  std::vector<char> _firstBuffer;
  std::vector<char> _secondBuffer;
  std::vector<char> _thirdBuffer;
  void setExpectedSize(size_t expectedSize) { _expectedSize = expectedSize; }
  std::vector<char>& getFirstBuffer(size_t capacity = 0);
  std::vector<char>& getSecondBuffer(size_t capacity = 0);
  std::vector<char>& getThirdBuffer(size_t capacity = 0);
  size_t getExpectedSize() const { return _expectedSize; }
  static void destroyBuffers();
 private:
  static MemoryPool& instance();
  // _expectedSize must be a static member,
  // otherwise it will be set only in
  // one and often irrelevant thread.
  // Instances of MemoryPool struct are
  // thread_local.
  static size_t _expectedSize;
};

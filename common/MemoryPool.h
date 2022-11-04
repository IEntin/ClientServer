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
  std::vector<char>& getFirstBuffer(size_t capacity = 0);
  std::vector<char>& getSecondBuffer(size_t capacity = 0);
  std::vector<char>& getThirdBuffer(size_t capacity = 0);
  static MemoryPool& instance();
  static void destroyBuffers();
 private:
  MemoryPool(const MemoryPool& other) = delete;
  MemoryPool& operator =(const MemoryPool& other) = delete;
};

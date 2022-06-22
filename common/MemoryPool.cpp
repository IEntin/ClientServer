/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Utility.h"

size_t MemoryPool::_expectedSize(0);

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

std::vector<char>& MemoryPool::getBuffer(size_t requested) {
  if (requested == 0)
    return instance()._buffer;
  else if (requested > instance()._buffer.capacity()) {
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << " increased _buffer from " << instance()._buffer.capacity()
      << " to " << requested << std::endl;
    instance()._buffer.reserve(requested);
  }
  return instance()._buffer;
}

void MemoryPool::destroyBuffer() {
  std::vector<char>& buffer = instance().getBuffer();
  CLOG <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << ' ' << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
}

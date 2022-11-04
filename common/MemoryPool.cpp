/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Utility.h"

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

std::vector<char>& MemoryPool::getFirstBuffer(size_t requested) {
  if (requested == 0)
    return instance()._firstBuffer;
  else if (requested > instance()._firstBuffer.capacity()) {
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << " increased _firstBuffer from " << instance()._firstBuffer.capacity()
      << " to " << requested << std::endl;
    instance()._firstBuffer.reserve(requested);
  }
  return instance()._firstBuffer;
}

std::vector<char>& MemoryPool::getSecondBuffer(size_t requested) {
  if (requested == 0)
    return instance()._secondBuffer;
  else if (requested > instance()._secondBuffer.capacity()) {
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << " increased _secondBuffer from " << instance()._secondBuffer.capacity()
      << " to " << requested << std::endl;
    instance()._secondBuffer.reserve(requested);
  }
  return instance()._secondBuffer;
}

std::vector<char>& MemoryPool::getThirdBuffer(size_t requested) {
  if (requested == 0)
    return instance()._thirdBuffer;
  else if (requested > instance()._thirdBuffer.capacity()) {
    CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << " increased _thirdBuffer from " << instance()._thirdBuffer.capacity()
      << " to " << requested << std::endl;
    instance()._thirdBuffer.reserve(requested);
  }
  return instance()._thirdBuffer;
}

void MemoryPool::destroyBuffers() {
  std::vector<char>& buffer = instance().getFirstBuffer();
  CLOG <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " first:" << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
  buffer = instance().getSecondBuffer();
  CLOG <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " second:" << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
  buffer = instance().getThirdBuffer();
  CLOG <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " third:" << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
}

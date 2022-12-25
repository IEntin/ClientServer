/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Logger.h"

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

std::vector<char>& MemoryPool::getFirstBuffer(size_t requested) {
  if (requested > _firstBuffer.capacity())
    Logger(LOG_LEVEL::DEBUG) << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << " increased _firstBuffer from " << _firstBuffer.capacity()
	 << " to " << requested << std::endl;
  _firstBuffer.reserve(requested);
  _firstBuffer.clear();
  return _firstBuffer;
}

std::vector<char>& MemoryPool::getSecondBuffer(size_t requested) {
  if (requested > _secondBuffer.capacity())
    Logger(LOG_LEVEL::DEBUG) << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << " increased _secondBuffer from " << _secondBuffer.capacity()
	 << " to " << requested << std::endl;
  _secondBuffer.reserve(requested);
  _secondBuffer.clear();
  return _secondBuffer;
}

void MemoryPool::destroyBuffers() {
  std::vector<char>& buffer = instance().getFirstBuffer();
  Logger(LOG_LEVEL::DEBUG) <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " first:" << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
  buffer = instance().getSecondBuffer();
  Logger(LOG_LEVEL::DEBUG) <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " second:" << buffer.capacity() << std::endl;
  std::vector<char>().swap(buffer);
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Compression.h"
#include "lz4.h"
#include <iostream>
#include <mutex>

namespace {

std::mutex logMutex;

} // end of anonimous namespace

size_t MemoryPool::_initialBufferSize(0);

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

void MemoryPool::setInitialSize(size_t initialBufferSize) {
  _initialBufferSize = initialBufferSize;
}

std::vector<char>& MemoryPool::getBuffer(size_t requested) {
  if (requested == 0)
    return instance()._buffer;
  else if (requested > instance()._buffer.capacity()) {
    {
      std::lock_guard lock(logMutex);
      std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< " increased _buffer from " << instance()._buffer.capacity()
		<< " to " << requested << std::endl;
    }
    instance()._buffer.reserve(requested);
  }
  return instance()._buffer;
}

void MemoryPool::resetBufferSize() {
  if (_perThreadBufferSize != _initialBufferSize) {
    {
      std::lock_guard lock(logMutex);
      std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
    }
    _buffer.resize(Compression::getCompressBound(_initialBufferSize));
    _buffer.shrink_to_fit();
    _perThreadBufferSize = _initialBufferSize;
  }
}

void MemoryPool::destroyBuffer() {
  std::vector<char>& buffer = instance().getBuffer();
  {
    std::lock_guard lock(logMutex);
    std::clog <<  __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ' ' << buffer.capacity() << std::endl;
  }
  std::vector<char>().swap(buffer);
}

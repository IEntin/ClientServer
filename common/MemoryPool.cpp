/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Compression.h"
#include "Header.h"
#include "lz4.h"
#include <iostream>

size_t MemoryPool::_initialBufferSize(0);

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

void MemoryPool::setInitialSize(size_t initialBufferSize) {
  _initialBufferSize = initialBufferSize;
}

std::vector<char>& MemoryPool::getBuffer(size_t requested) {
  instance().resetBufferSize();
  if (requested == 0)
    return instance()._buffer;
  else if (requested > instance()._buffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _buffer from " << instance()._buffer.capacity()
	      << " to " << requested << std::endl;
    instance()._buffer.reserve(requested);
  }
  return instance()._buffer;
}

void MemoryPool::resetBufferSize() {
  if (_perThreadBufferSize != _initialBufferSize) {
    _buffer.resize(Compression::getCompressBound(_initialBufferSize));
    _buffer.shrink_to_fit();
    _perThreadBufferSize = _initialBufferSize;
  }
}

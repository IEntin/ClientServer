/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Header.h"
#include "lz4.h"
#include <iostream>

size_t MemoryPool::_initialBufferSize(100000);

MemoryPool::MemoryPool() :
  _primaryBuffer(_initialBufferSize), _secondaryBuffer(_initialBufferSize) {}

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

void MemoryPool::setup(size_t initialBufferSize) {
  _initialBufferSize = initialBufferSize;
}

std::vector<char>& MemoryPool::getPrimaryBuffer(size_t requested) {
  if (requested == 0)
    return instance()._primaryBuffer;
  else if (requested > instance()._primaryBuffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _primaryBuffer from " << instance()._primaryBuffer.capacity()
	      << " to " << requested << std::endl;
    instance()._primaryBuffer.reserve(requested);
  }
  return instance()._primaryBuffer;
}

std::vector<char>& MemoryPool::getSecondaryBuffer(size_t requested) {
  if (requested == 0)
    return instance()._secondaryBuffer;
  else if (requested > instance()._secondaryBuffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _secondaryBuffer from " << instance()._secondaryBuffer.capacity()
	      << " to " << requested << std::endl;
    instance()._secondaryBuffer.reserve(requested);
  }
  return instance()._secondaryBuffer;
}

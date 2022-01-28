/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Header.h"
#include "lz4.h"
#include <iostream>

extern size_t getMemPoolBufferSize();

size_t MemoryPool::_initialBufferSize = getMemPoolBufferSize();
thread_local std::vector<char> MemoryPool::_primaryBuffer(LZ4_compressBound(_initialBufferSize) + HEADER_SIZE);
thread_local std::vector<char> MemoryPool::_secondaryBuffer(LZ4_compressBound(_initialBufferSize) + HEADER_SIZE);

std::pair<char*, size_t> MemoryPool::getPrimaryBuffer(size_t requested) {
  if (requested > _primaryBuffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _primaryBuffer from " << _primaryBuffer.capacity()
	      << " to " << requested << std::endl;
    _primaryBuffer.reserve(requested);
  }
  return { _primaryBuffer.data(), _primaryBuffer.capacity() };
}

std::vector<char>& MemoryPool::getSecondaryBuffer(size_t requested) {
  if (requested > _secondaryBuffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _secondaryBuffer from " << _secondaryBuffer.capacity()
	      << " to " << requested << std::endl;
    _secondaryBuffer.reserve(requested);
  }
  return _secondaryBuffer;
}

size_t MemoryPool::getInitialBufferSize() {
  return _initialBufferSize;
}

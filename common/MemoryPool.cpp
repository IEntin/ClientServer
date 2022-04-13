/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "MemoryPool.h"
#include "Compression.h"
#include "Header.h"
#include "lz4.h"

size_t MemoryPool::_initialBufferSize(0);

MemoryPool& MemoryPool::instance() {
  static thread_local MemoryPool instance;
  return instance;
}

void MemoryPool::setInitialSize(size_t initialBufferSize) {
  _initialBufferSize = initialBufferSize;
}

std::vector<char>& MemoryPool::getPrimaryBuffer(size_t requested) {
  instance().resetBufferSize();
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
  instance().resetBufferSize();
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

void MemoryPool::resetBufferSize() {
  if (_perThreadBufferSize != _initialBufferSize) {
    std::vector<char>().swap(_primaryBuffer);
    _primaryBuffer.resize(Compression::getCompressBound(_initialBufferSize));
    std::vector<char>().swap(_secondaryBuffer);
    _secondaryBuffer.resize(Compression::getCompressBound(_initialBufferSize));
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":buffer size reset from " << _perThreadBufferSize
	      <<" to " << _initialBufferSize << std::endl;
    _perThreadBufferSize = _initialBufferSize;
  }
}

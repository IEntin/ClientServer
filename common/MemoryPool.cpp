#include "MemoryPool.h"
#include "ProgramOptions.h"
#include <iostream>

const size_t DYNAMIC_BUFFER_SIZE = ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 200000);

thread_local std::vector<char> MemoryPool::_primaryBuffer(DYNAMIC_BUFFER_SIZE);
thread_local std::vector<char> MemoryPool::_secondaryBuffer(DYNAMIC_BUFFER_SIZE);

std::pair<char*, size_t> MemoryPool::getPrimaryBuffer(size_t requested) {
  if (requested > _primaryBuffer.capacity()) {
    std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " increased _primaryBuffer from " << _primaryBuffer.capacity()
	      << " to " << requested << std::endl;
    _primaryBuffer.reserve(requested);
  }
  return { &_primaryBuffer[0], _primaryBuffer.capacity() };
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

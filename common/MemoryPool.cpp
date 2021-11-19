#include "MemoryPool.h"
#include "lz4.h"

const size_t DYNAMIC_BUFFER_SIZE = ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 200000);

const size_t fixedBufferSize = std::max(static_cast<size_t>(LZ4_compressBound(DYNAMIC_BUFFER_SIZE)), DYNAMIC_BUFFER_SIZE);

thread_local std::vector<char> MemoryPool::_buffer(fixedBufferSize);
thread_local std::vector<char> MemoryPool::_receiveBuffer(fixedBufferSize);

std::pair<char*, size_t> MemoryPool::getBuffer() {
  return { &_buffer[0], _buffer.size() };
}

std::pair<char*, size_t> MemoryPool::getBuffer(size_t capacity) {
  if (capacity <= _buffer.size())
    return { &_buffer[0], _buffer.size() };
  else
    return { nullptr, 0 };
}

std::pair<char*, size_t> MemoryPool::getReceiveBuffer(size_t capacity) {
  if (capacity <= _receiveBuffer.size())
    return { &_receiveBuffer[0], _receiveBuffer.size() };
  else
    return { nullptr, 0 };
}

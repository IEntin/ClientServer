/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Lines.h"
#include "Logger.h"
#include <filesystem>

Lines::Lines(std::string_view fileName) {
  try {
    _fileSize = std::filesystem::file_size(fileName);
    _stream.open(fileName.data(), std::ios::binary);
    _stream.exceptions(std::ifstream::failbit); // may throw
    refillBuffer();
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
  }
}

// reentrant, thread safe
bool Lines::refillBuffer() {
  if (_buffer.capacity() == _buffer.size())
    return false;
  if (_stream) {
    size_t bytesToRead = std::min(_fileSize - _currentPos, _buffer.capacity() - _buffer.size());
    if (bytesToRead == 0)
      return false;
    _currentPos += bytesToRead;
    size_t shift = _buffer.size();
    _buffer.resize(_buffer.size() + bytesToRead);
    assert(_buffer.size() <= STATIC_VECTOR_SIZE && "Size exceeds const capacity");
    _stream.read(_buffer.data() + shift, bytesToRead);
    return true;
  }
  return false;
}

bool Lines::getLine(std::string_view& line) {
  if (_last)
    return false;
  try {
    if (_totalParsed < _fileSize && _processed >= _bufferRefillThreshold) {
      removeProcessedLines();
      refillBuffer();
    }
    auto itBeg = _buffer.begin() + _processed;
    auto itEnd = std::find(itBeg, _buffer.end(), '\n');
    bool endsWithDelimiter = itEnd != _buffer.end();
    if (endsWithDelimiter) {
      _totalParsed += std::distance(itBeg, itEnd + 1);
      ++_index;
      // include delimiter
      _currentLine = { itBeg, itEnd + 1 };
      line = _currentLine;
      _processed += _currentLine.size();
      if (_totalParsed == _fileSize)
	_last = true;
      return true;
    }
    else {
      _totalParsed += std::distance(itBeg, itEnd);
      if (_totalParsed == _fileSize) {
	++_index;
	_currentLine = { itBeg, itEnd };
	line = _currentLine;
	_processed += _currentLine.size();
	_last = true;
      }
      return true;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
  }
  return false;
}

void Lines::removeProcessedLines() {
  if (_processed == 0)
    return;
  std::memcpy(_buffer.data(), _buffer.data() + _processed, _buffer.size() - _processed);
  _buffer.resize(_buffer.size() - _processed);
  _processed = 0;
}

// reuse for another (or the same) file
void Lines::reset(std::string_view fileName) {
  _buffer.erase(_buffer.begin(), _buffer.end());
  _currentPos = 0;
  _processed = 0;
  _totalParsed = 0;
  _index = -1;
  _last = false;
  try {
    _stream.close();
    _fileSize = std::filesystem::file_size(fileName);
    _stream.open(fileName.data(), std::ios::binary);
    _stream.exceptions(std::ifstream::failbit); // may throw
    refillBuffer();
  }
  catch (const std::ios_base::failure& fail) {
    std::cout << fail.what() << '\n';
  }
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FileLines.h"

#include <filesystem>

FileLines::FileLines(std::string_view fileName, char delimiter, bool keepDelimiter) :
  Lines(delimiter, keepDelimiter) {
  _inputSize = std::filesystem::file_size(fileName);
  _stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  _stream.open(fileName.data(), std::ios::binary);
  refillBuffer();
}

std::size_t FileLines::getInputPosition() {
  return _stream.tellg();
}

bool FileLines::refillBuffer() {
  std::size_t bytesToRead = std::min(_inputSize - getInputPosition(), _buffer.size() - _sizeInUse);
  _stream.read(_buffer.data() + _sizeInUse, bytesToRead);
  _sizeInUse += _stream.gcount();
  return true;
}

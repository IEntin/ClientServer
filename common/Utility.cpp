/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"

#include <atomic>
#include <filesystem>
#include <fstream>

#include "IOUtility.h"
#include "Logger.h"

namespace utility {

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    LogError << std::strerror(errno) << '\n';
  _fd = -1;
}

std::size_t getUniqueId() {
  static std::atomic<size_t> uniqueInteger;
  return uniqueInteger.fetch_add(1);
}

void readFile(std::string_view fileName, std::string& buffer) {
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  std::uintmax_t size = std::filesystem::file_size(fileName);
  buffer.resize(size);
  stream.read(buffer.data(), buffer.size());
}

// used in tests
bool getLastLine(std::string_view fileName, std::string& lastLine) {
  char ch;
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  stream.seekg(-2, std::ios_base::end);
  stream.get(ch);
  while (ch != '\n') {
    stream.seekg(-2, std::ios_base::cur);
    stream.get(ch);
  }
  std::getline(stream, lastLine);
  return true;
}

// used in tests
bool fileEndsWithEOL(std::string_view fileName) {
  char ch = 'x';
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  stream.seekg(-1, std::ios::end);
  stream.get(ch);
  return ch == '\n';
}

std::string createErrorString(std::errc ec,
			      const boost::source_location& location) {
  std::string msg(std::make_error_code(ec).message());
  msg.append(1, ':').append(location.file_name()).append(1, ':');
  ioutility::toChars(location.line(), msg);
  return msg.append(1, ' ').append(location.function_name());
}

std::string createErrorString(const boost::source_location& location) {
  std::string msg(std::strerror(errno));
  msg.append(1, ':').append(location.file_name()).append(1, ':');
  ioutility::toChars(location.line(), msg);
  return msg.append(1, ' ').append(location.function_name());
}

} // end of namespace utility

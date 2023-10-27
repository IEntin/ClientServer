/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <filesystem>
#include <fstream>
#include <mutex>

namespace utility {

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    LogError << std::strerror(errno) << '\n';
  _fd = -1;
}

std::string getUniqueId() {
  try {
    static boost::uuids::random_generator generator;
    static std::mutex mutex;
    std::lock_guard lock(mutex);
    auto uuid = generator();
    std::string str = boost::uuids::to_string(uuid);
    auto it = std::remove(str.begin(), str.end(), '-');
    return { str.begin(), it };
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    return "";
  }
}

void readFile(std::string_view name, std::string& buffer) {
  try {
    std::ifstream stream(name.data(), std::ios::binary);
    stream.exceptions(std::ofstream::failbit);
    std::uintmax_t size = std::filesystem::file_size(name);
    buffer.resize(size);
    stream.read(buffer.data(), buffer.size());
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
  }
}

bool writeFile(std::string_view name, std::string_view contents) {
  try {
    std::ofstream stream(name.data(), std::ios::binary);
    stream.exceptions(std::ofstream::failbit);
    stream.write(contents.data(), contents.size());
    return true;
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
    return false;
  }
}

// used in tests
bool getLastLine(std::string_view fileName, std::string& lastLine) {
  char ch;
  try {
    std::ifstream stream(fileName.data(), std::ios::binary);
    stream.exceptions(std::ifstream::failbit);
    stream.seekg(-2, std::ios_base::end);
    stream.get(ch);
    while (ch != '\n') {
      stream.seekg(-2, std::ios_base::cur);
      stream.get(ch);
    }
    std::getline(stream, lastLine);
    return true;
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
    return false;
  }
}

// used in tests
bool fileEndsWithEOL(std::string_view fileName) {
  char ch = 'x';
  try {
    std::ifstream stream(fileName.data(), std::ios::binary);
    stream.exceptions(std::ifstream::failbit);
    stream.seekg(-1, std::ios::end);
    stream.get(ch);
    return ch == '\n';
  }
  catch (const std::ios_base::failure& fail) {
    LogError << fail.what() << '\n';
    return false;
  }
}

} // end of namespace utility

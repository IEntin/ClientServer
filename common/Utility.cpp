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
  std::ifstream ifs(name.data(), std::ios::binary);
  if (ifs) {
    std::uintmax_t size = std::filesystem::file_size(name);
    buffer.resize(size);
    ifs.read(buffer.data(), buffer.size());
  }
}

bool writeFile(std::string_view name, std::string_view contents) {
  std::ofstream ofs(name.data(), std::ios::binary);
  if (ofs) {
    ofs.write(contents.data(), contents.size());
    if (ofs)
      return true;
  }
  return false;
}

} // end of namespace utility

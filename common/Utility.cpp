/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include <cstring>
#include <iostream>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace utility {

std::string readFile(const std::string& name) {
  std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

void readFile(const std::string& name, std::vector<char>& buffer) {
  std::uintmax_t size = std::filesystem::file_size(name);
  buffer.resize(size);
  int fd = open(name.data(), O_RDONLY);
  if (fd == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
  ssize_t result = read(fd, buffer.data(), size);
  if (result == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
}

} // end of namespace utility

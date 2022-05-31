/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

namespace utility {

struct CloseFile {
  CloseFile(int& fd) : _fd(fd) {}
  ~CloseFile() {
    if (_fd != -1)
      close(_fd);
  }
  int& _fd;
};

std::string readFile(const std::string& name) {
  std::vector<char> buffer;
  readFile(name, buffer);
  return { std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()) };
}

void readFile(const std::string& name, std::vector<char>& buffer) {
  std::uintmax_t size = std::filesystem::file_size(name);
  buffer.resize(size);
  int fd = -1;
  CloseFile cf(fd);
  fd = open(name.data(), O_RDONLY);
  if (fd == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
  ssize_t result = read(fd, buffer.data(), size);
  if (result == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
}

} // end of namespace utility

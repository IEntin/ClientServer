/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include "Header.h"
#include <fcntl.h>
#include <filesystem>
#include <unistd.h>

namespace utility {

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << ':' << std::strerror(errno) << std::endl;
  _fd = -1;
}

std::string readFile(const std::string& name) {
  std::vector<char> buffer;
  readFile(name, buffer);
  return { std::make_move_iterator(buffer.begin()), std::make_move_iterator(buffer.end()) };
}

void readFile(const std::string& name, std::vector<char>& buffer) {
  std::uintmax_t size = std::filesystem::file_size(name);
  buffer.resize(size);
  int fd = -1;
  CloseFileDescriptor cfd(fd);
  fd = open(name.data(), O_RDONLY);
  if (fd == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
  ssize_t result = read(fd, buffer.data(), size);
  if (result == -1)
    throw std::runtime_error(std::string(std::strerror(errno)) + ':' + name);
}

bool displayStatus(STATUS status) {
  switch (status) {
  case STATUS::NONE:
    return false;
  case STATUS::BAD_HEADER:
    CERR << "STATUS::BAD_HEADER" << std::endl;
    return true;
  case STATUS::COMPRESSION_PROBLEM:
    CERR << "STATUS::COMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::DECOMPRESSION_PROBLEM:
    CERR << "STATUS::DECOMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::FIFO_PROBLEM:
    CERR << "STATUS::FIFO_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_PROBLEM:
    CERR << "STATUS::TCP_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_TIMEOUT:
    CERR << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json" << std::endl;
    return false;
  case STATUS::MAX_TOTAL_SESSIONS:
    CLOG << "STATUS::MAX_TOTAL_SESSIONS" << std::endl;
    return false;
  case STATUS::MAX_NUMBER_RUNNABLES:
    CLOG << "STATUS::MAX_NUMBER_RUNNABLES" << std::endl;
    return false;
  default:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":unexpected problem" << std::endl;
    return true;
  }
}

} // end of namespace utility

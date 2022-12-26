/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include "Header.h"
#include "Logger.h"
#include <fcntl.h>
#include <filesystem>
#include <sys/resource.h>
#include <unistd.h>

namespace utility {

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__
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
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::BAD_HEADER" << std::endl;
    return true;
  case STATUS::COMPRESSION_PROBLEM:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::COMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::DECOMPRESSION_PROBLEM:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::DECOMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::FIFO_PROBLEM:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::FIFO_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_PROBLEM:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::TCP_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_TIMEOUT:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json" << std::endl;
    return true;
 case STATUS::HEARTBEAT_PROBLEM:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "STATUS::HEARTBEAT_PROBLEM" << std::endl;
    return true;
 case STATUS::HEARTBEAT_TIMEOUT:
    Logger(LOG_LEVEL::ERROR, std::cerr) << "\theartbeat timeout! Increase \"HeartbeatTimeout\" in ClientOptions.json" << std::endl;
    return true;
  case STATUS::MAX_TOTAL_SESSIONS:
    Logger(LOG_LEVEL::WARN) << "STATUS::MAX_TOTAL_SESSIONS" << std::endl;
    return false;
  case STATUS::MAX_SPECIFIC_SESSIONS:
    Logger(LOG_LEVEL::WARN) << "STATUS::MAX_SPECIFIC_SESSIONS" << std::endl;
    return false;
  default:
    Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__
      << ":unexpected problem" << std::endl;
    return true;
  }
}

} // end of namespace utility

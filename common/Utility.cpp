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
    LogError << std::strerror(errno) << std::endl;
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
    LogError << "STATUS::BAD_HEADER" << std::endl;
    return true;
  case STATUS::COMPRESSION_PROBLEM:
    LogError << "STATUS::COMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::DECOMPRESSION_PROBLEM:
    LogError << "STATUS::DECOMPRESSION_PROBLEM" << std::endl;
    return true;
  case STATUS::FIFO_PROBLEM:
    LogError << "STATUS::FIFO_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_PROBLEM:
    LogError << "STATUS::TCP_PROBLEM" << std::endl;
    return true;
  case STATUS::TCP_TIMEOUT:
    LogError << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json" << std::endl;
    return true;
 case STATUS::HEARTBEAT_PROBLEM:
    LogError << "STATUS::HEARTBEAT_PROBLEM" << std::endl;
    return true;
 case STATUS::HEARTBEAT_TIMEOUT:
    LogError << "\theartbeat timeout! Increase \"HeartbeatTimeout\" in ClientOptions.json" << std::endl;
    return true;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "Exceeded max total number clients" << std::endl;
    return false;
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "Exceeded max number clients of type" << std::endl;
    return false;
  case STATUS::ENCRYPTION_PROBLEM:
    Warn << "Encryption problem" << std::endl;
    return false;
  case STATUS::DECRYPTION_PROBLEM:
    Warn << "Decryption problem" << std::endl;
    return false;
  case STATUS::ERROR:
    Warn << "Internal error" << std::endl;
    return false;
  default:
    LogError << "unexpected problem" << std::endl;
    return true;
  }
}

void displayMaxTotalSessionsWarn() {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe total number of sessions exceeds system capacity.\n"
       << "\tThis client will wait in the queue until load subsides.\n"
       << "\tIt will start running if some other clients are closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"MaxTotalSessions\" in ServerOptions.json.\n"
       << "\t!!!!!!!!!" << std::endl;
}

void displayMaxSessionsOfTypeWarn(std::string_view type) {
  Warn << "\n\t!!!!!!!!!\n"
       << "\tThe number of " << type << " sessions exceeds pool capacity.\n"
       << "\tThis client is waiting in the queue for available thread.\n"
       << "\tIt will start running if some other " << type << " clients\n"
       << "\tare closed.\n"
       << "\tYou can also close this client and try again later,\n"
       << "\tbut spot in the queue will be lost.\n"
       << "\tSee \"Max" << (type == "fifo" ? "Fifo" : "Tcp") << "Sessions\""
       << " in ServerOptions.json.\n"
       << "\t!!!!!!!!!" << std::endl;
}

} // end of namespace utility

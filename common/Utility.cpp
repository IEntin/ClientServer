/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include "Header.h"
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <fcntl.h>
#include <filesystem>
#include <mutex>
#include <sys/resource.h>
#include <unistd.h>

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
    std::scoped_lock lock(mutex);
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

std::string readFile(const std::string& name) {
  std::ostringstream buffer;
  std::ifstream ifs(name, std::ios::binary);
  if (ifs) {
    buffer << ifs.rdbuf();
  }
  return buffer.str();
}

bool writeFile(const std::string& name, const std::string& contents) {
  std::ofstream ofs(name, std::ios::binary);
  if (ofs) {
    ofs << contents;
    return true;
  }
  return false;
}

bool displayStatus(STATUS status) {
  switch (status) {
  case STATUS::NONE:
    return false;
  case STATUS::BAD_HEADER:
    LogError << "STATUS::BAD_HEADER" << '\n';
    return true;
  case STATUS::COMPRESSION_PROBLEM:
    LogError << "STATUS::COMPRESSION_PROBLEM" << '\n';
    return true;
  case STATUS::DECOMPRESSION_PROBLEM:
    LogError << "STATUS::DECOMPRESSION_PROBLEM" << '\n';
    return true;
  case STATUS::FIFO_PROBLEM:
    LogError << "STATUS::FIFO_PROBLEM" << '\n';
    return true;
  case STATUS::TCP_PROBLEM:
    LogError << "STATUS::TCP_PROBLEM" << '\n';
    return true;
  case STATUS::TCP_TIMEOUT:
    LogError << "\tserver timeout! Increase \"TcpTimeout\" in ServerOptions.json" << '\n';
    return true;
 case STATUS::HEARTBEAT_PROBLEM:
    LogError << "STATUS::HEARTBEAT_PROBLEM" << '\n';
    return true;
 case STATUS::HEARTBEAT_TIMEOUT:
    LogError << "\theartbeat timeout! Increase \"HeartbeatTimeout\" in ClientOptions.json" << '\n';
    return true;
  case STATUS::MAX_TOTAL_OBJECTS:
    Warn << "Exceeded max total number clients" << '\n';
    return false;
  case STATUS::MAX_OBJECTS_OF_TYPE:
    Warn << "Exceeded max number clients of type" << '\n';
    return false;
  case STATUS::ENCRYPTION_PROBLEM:
    Warn << "Encryption problem" << '\n';
    return false;
  case STATUS::DECRYPTION_PROBLEM:
    Warn << "Decryption problem" << '\n';
    return false;
  case STATUS::ERROR:
    Warn << "Internal error" << '\n';
    return false;
  default:
    LogError << "unexpected problem" << '\n';
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
       << "\t!!!!!!!!!" << '\n';
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
       << "\t!!!!!!!!!" << '\n';
}

} // end of namespace utility

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "CommonConstants.h"
#include "Fifo.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <fcntl.h>
#include <filesystem>

namespace fifo {

namespace {
std::string fifoName;
} // end of anonimous namespace

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  try {
    boost::interprocess::named_mutex wakeupMutex{ boost::interprocess::open_or_create, WAKEUP_MUTEX };
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock{ wakeupMutex };
    if (!wakeupAcceptor())
      return;
    if (!receiveStatus())
      throw std::runtime_error("FifoClient::receiveStatus failed");
    fifoName = _fifoName;
  }
  catch (boost::interprocess::interprocess_exception& e) {
    LogError << e.what() << std::endl;
    throw std::runtime_error("named mutex lock failure.");
  }
}

FifoClient::~FifoClient() {
  Fifo::onExit(_fifoName, _options);
  Trace << std::endl;
}

bool FifoClient::run() {
  start();
  return Client::run();
}

bool FifoClient::send(const Subtask& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  _fdWrite = open(_fifoName.data(), O_WRONLY);
  if (_stopFlag.test())
    return false;
  if (!std::filesystem::exists(_options._controlFileName))
    return false;
  if (_fdWrite >= 0) {
    if (_options._setPipeSize)
      Fifo::setPipeSize(_fdWrite, subtask._body.size());
    std::string_view body(subtask._body.data(), subtask._body.size());
    return Fifo::sendMsg(_fdWrite, subtask._header, body);
  }
  return false;
}

bool FifoClient::receive() {
  if (!std::filesystem::exists(_options._controlFileName))
    return false;
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    LogError << _fifoName << '-' << std::strerror(errno) << std::endl;
    return false;
  }
  _status = STATUS::NONE;
  try {
    HEADER header = Fifo::readHeader(_fdRead, _options);
    if (!readReply(header)) {
      LogError << "failed." << std::endl;
      return false;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::readReply(const HEADER& header) {
  thread_local static std::vector<char> buffer;
  ssize_t comprSize = extractCompressedSize(header);
  buffer.reserve(comprSize);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize, _options)) {
    LogError << "failed." << std::endl;
    return false;
  }
  return printReply(buffer, header);
}

bool FifoClient::wakeupAcceptor() {
  int fd = -1;
  utility::CloseFileDescriptor cfdw(fd);
  fd = open(_options._acceptorName.data(), O_WRONLY);
  if (fd == -1) {
    LogError << std::strerror(errno) << ' '
	     << _options._acceptorName << std::endl;
    return false;
  }
  HEADER header = { HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, _status };
  return Fifo::sendMsg(fd, header);
}

bool FifoClient::receiveStatus() {
  try {
    int fd = -1;
    utility::CloseFileDescriptor closefd(fd);
    fd = open(_options._acceptorName.data(), O_RDONLY);
    if (fd == -1) {
      LogError << std::strerror(errno) << ' '
	       << _options._acceptorName << std::endl;
      return false;
    }
    HEADER header = Fifo::readHeader(fd, _options);
    size_t size = extractUncompressedSize(header);
    _fifoName.resize(size);
    if (!Fifo::readString(fd, _fifoName.data(), size, _options)) {
      LogError << "failed." << std::endl;
      return false;
    }
    _clientId = std::filesystem::path(_fifoName).filename();
    _status = extractStatus(header);
    switch (_status) {
    case STATUS::NONE:
      break;
    case STATUS::MAX_SPECIFIC_OBJECTS:
      utility::displayMaxSpecificSessionsWarn("fifo");
      break;
    case STATUS::MAX_TOTAL_OBJECTS:
      utility::displayMaxTotalSessionsWarn();
      break;
    default:
      break;
    }
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::destroySession() {
  int fd = -1;
  utility::CloseFileDescriptor cfdw(fd);
  fd = Fifo::openWriteEndNonBlock(_options._acceptorName, _options);
  if (fd == -1)
    return false;
  size_t size = _clientId.size();
  HEADER header = { HEADERTYPE::DESTROY_SESSION, size, size, COMPRESSORS::NONE, false, _status };
  return Fifo::sendMsg(fd, header, _clientId);
}

void FifoClient::setStopFlag(const ClientOptions& options) {
  int old_errno = errno;
  _stopFlag.test_and_set();
  Fifo::openWriteEndNonBlock(fifoName, options) ;
  Fifo::openReadEndNonBlock(fifoName, options);
  errno = old_errno;
}

} // end of namespace fifo

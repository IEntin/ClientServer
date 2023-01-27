/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "CommonConstants.h"
#include "Fifo.h"
#include "MemoryPool.h"
#include "Metrics.h"
#include "Utility.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <fcntl.h>
#include <filesystem>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  try {
    boost::interprocess::named_mutex wakeupMutex{ boost::interprocess::open_or_create, WAKEUP_MUTEX };
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock{ wakeupMutex };
    if (!wakeupAcceptor())
      return;
    if (!receiveStatus())
      throw std::runtime_error("FifoClient::receiveStatus failed");
  }
  catch (boost::interprocess::interprocess_exception& e) {
    LogError << e.what() << std::endl;
    throw std::runtime_error("named mutex lock failure.");
  }
}

FifoClient::~FifoClient() {
  Metrics::save();
  Fifo::onExit(_fifoName, _options);
  Trace << std::endl;
}

bool FifoClient::run() {
  start();
  return Client::run();
}

bool FifoClient::send(const std::vector<char>& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  while (_fdWrite == -1) {
    int rep = 0;
    do {
      _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
      if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
    } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
    if (_fdWrite >= 0) {
      if (_options._setPipeSize)
	Fifo::setPipeSize(_fdWrite, subtask.size());
      if (_stopFlag.test())
	return false;
      return Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()));
    }
    // server stopped
    if (!std::filesystem::exists(_fifoName))
      return false;
    // client closed
    if (_stopFlag.test())
      return false;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
  return false;
}

bool FifoClient::receive() {
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
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, _status);
  return Fifo::writeString(fd, std::string_view(buffer, HEADER_SIZE)); 
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
    _clientId.resize(size);
    if (!Fifo::readString(fd, _clientId.data(), size, _options)) {
      LogError << "failed." << std::endl;
      return false;
    }
    _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(_clientId);
    _status = extractStatus(header);
    switch (_status) {
    case STATUS::NONE:
      break;
    case STATUS::MAX_SPECIFIC_SESSIONS:
      utility::displayMaxSpecificSessionsWarn("fifo");
      break;
    case STATUS::MAX_TOTAL_SESSIONS:
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
  int rep = 0;
  do {
    fd = open(_options._acceptorName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      switch (errno) {
      case ENXIO:
      case EINTR:
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
	break;
      default:
	LogError << std::strerror(errno) << std::endl;
	return false;
	break;
      }
    }
  } while (fd == -1 && rep++ < _options._numberRepeatENXIO);
  if (fd == -1) {
    LogError << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
    return false;
  }
  size_t size = _clientId.size();
  std::vector<char> buffer(HEADER_SIZE + size);
  encodeHeader(buffer.data(), HEADERTYPE::DESTROY_SESSION, size, size, COMPRESSORS::NONE, false, _status);
  std::copy(_clientId.cbegin(), _clientId.cend(), buffer.data() + HEADER_SIZE);
  return Fifo::writeString(fd, std::string_view(buffer.data(), HEADER_SIZE + _clientId.size()));
}

} // end of namespace fifo

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <fcntl.h>
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  try {
    boost::interprocess::named_mutex mutex(boost::interprocess::open_or_create, FIFO_NAMED_MUTEX);
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(mutex);
    if (!wakeupAcceptor())
      throw std::runtime_error("FifoClient::wakeupAcceptor failed");
    if (!receiveStatus())
      throw std::runtime_error("FifoClient::receiveStatus failed");
  }
  catch (const boost::interprocess::interprocess_exception& e) {
    LogError << e.what() << std::endl;
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
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  while (fdWrite == -1) {
    fdWrite = Fifo::openWriteNonBlock(_fifoName, _options);
    if (fdWrite >= 0) {
      if (_options._setPipeSize)
	Fifo::setPipeSize(fdWrite, subtask._body.size());
      if (_closeFlag.test())
	return false;
      std::string_view body(subtask._body.data(), subtask._body.size());
      return Fifo::sendMsg(fdWrite, subtask._header, body);
   }
    // server stopped
    if (!std::filesystem::exists(_fifoName))
      return false;
    // client closed
    if (_closeFlag.test())
      return false;
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }
  return false;
}

bool FifoClient::receive() {
  if (_closeFlag.test())
    return false;
  if (!std::filesystem::exists(_options._controlFileName))
    return false;
  _status = STATUS::NONE;
  try {
    thread_local static std::vector<char> buffer;
    HEADER header;
    if (!Fifo::readMsgBlock(_fifoName, header, buffer))
      return false;
    return printReply(buffer, header);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::wakeupAcceptor() {
  int fd = Fifo::openWriteNonBlock(_options._acceptorName, _options);
  utility::CloseFileDescriptor cfdw(fd);
  if (fd == -1) {
    LogError << std::strerror(errno) << ' ' << _options._acceptorName << std::endl;
    return false;
  }
  HEADER header =
    { HEADERTYPE::CREATE_SESSION, 0, 0, COMPRESSORS::NONE, false, _status };
  return Fifo::sendMsg(fd, header);
}

bool FifoClient::receiveStatus() {
  try {
    HEADER header;
    std::vector<char> buffer;
    if (!Fifo::readMsgBlock(_options._acceptorName, header, buffer))
      return false;
    _clientId.assign(buffer.begin(), buffer.end());
    _status = extractStatus(header);
    _fifoName = _options._fifoDirectoryName + '/' + _clientId;
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

bool FifoClient::destroy(const ClientOptions& options) {
  int fd = Fifo::openWriteNonBlock(options._acceptorName, options);
  utility::CloseFileDescriptor cfdw(fd);
  if (fd == -1)
    return false;
  size_t size = _clientId.size();
  HEADER header = { HEADERTYPE::DESTROY_SESSION, size, size, COMPRESSORS::NONE, false, STATUS::NONE };
  return Fifo::sendMsg(fd, header, _clientId);
}

void FifoClient::setCloseFlag(const ClientOptions& options) {
  // prevent spoiled errno
  int old_errno = errno;
  _closeFlag.test_and_set();
  // calling only open and write
  destroy(options);
  errno = old_errno;
}

} // end of namespace fifo

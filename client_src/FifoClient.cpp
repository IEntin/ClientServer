/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "CommonNames.h"
#include "Fifo.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <fcntl.h>
#include <filesystem>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  boost::interprocess::named_mutex wakeupMutex{ boost::interprocess::open_or_create, WAKEUP_MUTEX };
  boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock{ wakeupMutex };
  if (!wakeupAcceptor())
    return;
  receiveStatus();
}

FifoClient::~FifoClient() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoClient::send(const std::vector<char>& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  unsigned numberSeconds = 0;
  while (_fdWrite == -1) {
    int rep = 0;
    do {
      _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
      if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
    } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
    if (_fdWrite != -1) {
      if (_options._setPipeSize)
	Fifo::setPipeSize(_fdWrite, subtask.size());
      if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
	CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << '\n';
	return false;
      }
      return true;
    }
    // server stopped
    if (!std::filesystem::exists(_fifoName))
      return false;
    // client closed
    if (_stopFlag.test()) {
      std::filesystem::remove(_fifoName);
      return false;
    }
    std::this_thread::sleep_for(std::chrono::seconds(1));
    if (++numberSeconds == 5) {
      CLOG << '.' << std::flush;
      numberSeconds = 0;
    }
  }
  CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
       << _fifoName << '-' << std::strerror(errno) << '\n';
  return false;
}

bool FifoClient::receive() {
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << _fifoName << '-' << std::strerror(errno) << '\n';
    return false;
  }
  HEADER header = Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  if (!readReply(header)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return false;
  }
  return true;
}

bool FifoClient::readReply(const HEADER& header) {
  thread_local static std::vector<char> buffer;
  ssize_t comprSize = getCompressedSize(header);
  buffer.reserve(comprSize);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize, _options._numberRepeatEINTR)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return false;
  }
  return printReply(buffer, header);
}

bool FifoClient::wakeupAcceptor() {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  _fdWrite = open(_options._acceptorName.data(), O_WRONLY);
  if (_fdWrite == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
    return false;
  }
  return true;
}

bool FifoClient::receiveStatus() {
  int fd = -1;
  utility::CloseFileDescriptor closefd(fd);
  fd = open(_options._acceptorName.data(), O_RDONLY);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
    return false;
  }
  HEADER header = Fifo::readHeader(fd, _options._numberRepeatEINTR);
  size_t size = getUncompressedSize(header);
  std::vector<char> buffer(size);
  if (!Fifo::readString(fd, buffer.data(), size, _options._numberRepeatEINTR)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return false;
  }
  _fifoName.assign(buffer.data(), size);
  _status = getProblem(header);
  switch (_status) {
  case STATUS::NONE:
    break;
  case STATUS::MAX_NUMBER_RUNNABLES:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running fifo sessions exceeds thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\ta thread available after one of already running fifo clients\n"
	 << "\tis closed. At this point the client will resume the run.\n"
	 << "\tYou can also close the client and try again later.\n"
	 << "\tThe relevant setting is \"MaxFifoSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!\n";
    break;
  case STATUS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
  return true;
}

} // end of namespace fifo

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <csignal>
#include <fcntl.h>
#include <filesystem>
#include <sys/types.h>
#include <sys/stat.h>

extern volatile std::sig_atomic_t stopSignal;

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options),
  _clientId(utility::getUniqueId()) {
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(_clientId);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__
       << " _fifoName:" << _fifoName << std::endl;
  if (mkfifo(_fifoName.data(), 0620) == -1 && errno != EEXIST) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << '-' << _fifoName << '\n';
    return;
  }
  if (!sendClientId())
    return;
  if (!receiveStatus())
    return;
}

FifoClient::~FifoClient() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoClient::send(const std::vector<char>& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  // already running
  if (_running.test_and_set() || _problem == PROBLEMS::NONE) {
    int rep = 0;
    do {
      _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
      if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
    } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  }
  // waiting in queue
  else {
    unsigned numberSesonds = 0;
    while (_fdWrite == -1) {
      int rep = 0;
      do {
	_fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
	if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
	  std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
      } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
      // server stopped
      if (!std::filesystem::exists(_fifoName))
	break;
      // client closed
      if (stopSignal) {
	std::filesystem::remove(_fifoName);
	break;
      }
      if (_fdWrite == -1) {
	std::this_thread::sleep_for(std::chrono::seconds(1));
	++numberSesonds;
	if (numberSesonds % 5 == 0) {
	  CLOG << '.' << std::flush;
	  numberSesonds = 0;
	}
      }
    }
  }
  if (_fdWrite == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << '-' << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  if (_options._setPipeSize)
    Fifo::setPipeSize(_fdWrite, subtask.size());
  if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << '\n';
    return false;
  }
  return true;
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

bool FifoClient::sendClientId() {
  int fd = -1;
  size_t requestedSize = _clientId.size() + HEADER_SIZE;
  std::vector<char>& buffer = MemoryPool::instance().getSecondBuffer(requestedSize);
  buffer.resize(requestedSize);
  encodeHeader(buffer.data(),
	       HEADERTYPE::CLIENTID,
	       _clientId.size(),
	       _clientId.size(),
	       COMPRESSORS::NONE,
	       false,
	       0,
	       PROBLEMS::NONE);
  std::copy(_clientId.begin(), _clientId.end(), buffer.begin() + HEADER_SIZE);
  {
    utility::CloseFileDescriptor closefd(fd);
    int rep = 0;
    do {
      fd = open(_options._acceptorName.data(), O_WRONLY | O_NONBLOCK);
      if (fd == -1 && (errno == ENXIO || errno == EINTR))
	std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
    } while (fd == -1 &&
	     (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
    if (fd == -1) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	   << std::strerror(errno) << ' ' << _options._acceptorName << '\n';
      return false;
    }
    if (!Fifo::writeString(fd, std::string_view(buffer.data(), buffer.size()))) {
      CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":acceptor failure\n";
      return false;
    }
  }
  return true;
}

bool FifoClient::receiveStatus() {
  int fd = -1;
  utility::CloseFileDescriptor closefd(fd);
  fd = open(_fifoName.data(), O_RDONLY);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  HEADER header = Fifo::readHeader(fd, _options._numberRepeatEINTR);
  _problem = getProblem(header);
  switch (_problem) {
  case PROBLEMS::NONE :
    break;
  case PROBLEMS::MAX_NUMBER_RUNNABLES:
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << "\n\t!!!!!!!!!\n"
	 << "\tThe number of running fifo sessions is at thread pool capacity.\n"
	 << "\tIf you do not close the client, it will wait in the queue for\n"
	 << "\ta thread available after one of already running fifo clients\n"
	 << "\tis closed. At this point the client will resume the run.\n"
	 << "\tYou can also close the client and try again later.\n"
	 << "\tThe relevant setting is \"MaxFifoSessions\" in ServerOptions.json.\n"
	 << "\t!!!!!!!!!\n";
    break;
  case PROBLEMS::MAX_TOTAL_SESSIONS:
    // TBD
    break;
  default:
    break;
  }
  return true;
}

} // end of namespace fifo

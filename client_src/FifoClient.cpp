/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoClient.h"
#include "ClientOptions.h"
#include "Fifo.h"
#include "Header.h"
#include "Utility.h"
#include <cstring>
#include <fcntl.h>

namespace fifo {

namespace {

std::string_view denial("\t\t!!!!!!!!!!!!!!\n\tThe server is busy at the moment.\n"
			"\t\tTry again later.\n\t\t!!!!!!!!!!!!!!\n");

} // end of anonimous namespace

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options) {
  // wake up acceptor
  int fd = -1;
  utility::CloseFileDescriptor closefd(fd);
  int rep = 0;
  do {
    fd = open(options._acceptorName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
  } while (fd == -1 &&
	   (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	 << std::strerror(errno) << ' ' << options._acceptorName << '\n';
  }
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, 0, 0, COMPRESSORS::NONE, false, 0, 'R', PROBLEMS::NONE);
  std::string_view view(buffer, HEADER_SIZE);
  if (!Fifo::writeString(fd, view))
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":write failed\n";
  // receive status
  HEADER header;
  close(fd);
  fd = -1;
  fd = open(options._acceptorName.data(), O_RDONLY);
  if (fd == -1) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-' 
	 << std::strerror(errno) << ' ' << options._acceptorName << '\n';
    return;
  }
  header = Fifo::readHeader(fd, _options._numberRepeatEINTR);
  _problem = getProblem(header);
  _ephemeralIndex = getEphemeral(header);
  char array[5] = {};
  std::string_view baseName = utility::toStringView(_ephemeralIndex, array, sizeof(array)); 
  _fifoName.append(_options._fifoDirectoryName).append(1,'/').append(baseName.data(), baseName.size());
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " _ephemeralIndex:"
       << _ephemeralIndex << ", _fifoName =" << _fifoName << '\n';
  if (_problem == PROBLEMS::MAX_FIFO_SESSIONS)
    CERR << denial;
}

FifoClient::~FifoClient() {
  Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '\n';
}

bool FifoClient::send(const std::vector<char>& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  int rep = 0;
  do {
    _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::milliseconds(_options._ENXIOwait));
  } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
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
  auto [uncomprSize, comprSize, compressor, diagnostics, ephemeral, clientId, problem] =
    Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  if (problem != PROBLEMS::NONE)
    return false;
  if (!readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return false;
  }
  return true;
}

bool FifoClient::readReply(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  thread_local static std::vector<char> buffer(comprSize + 1);
  buffer.reserve(comprSize);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize, _options._numberRepeatEINTR)) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed.\n";
    return false;
  }
  return printReply(buffer, uncomprSize, comprSize, bcompressed);
}

} // end of namespace fifo

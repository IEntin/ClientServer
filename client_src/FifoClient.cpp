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
#include <iostream>

namespace fifo {

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options), _fifoName(options._fifoName), _setPipeSize(options._setPipeSize) {}

FifoClient::~FifoClient() {
  std::clog << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
}

bool FifoClient::send(const std::vector<char>& subtask) {
  utility::CloseFileDescriptor cfdw(_fdWrite);
  int rep = 0;
  do {
    _fdWrite = open(_fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (_fdWrite == -1 && (errno == ENXIO || errno == EINTR))
      std::this_thread::sleep_for(std::chrono::microseconds(_options._ENXIOwait));
  } while (_fdWrite == -1 && (errno == ENXIO || errno == EINTR) && rep++ < _options._numberRepeatENXIO);
  if (_fdWrite == -1) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << ' ' << _fifoName << std::endl;
    return false;
  }
  if (_setPipeSize)
    Fifo::setPipeSize(_fdWrite, subtask.size());
  if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::receive() {
  utility::CloseFileDescriptor cfdr(_fdRead);
  _fdRead = open(_fifoName.data(), O_RDONLY);
  if (_fdRead == -1) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << _fifoName << '-' << std::strerror(errno) << std::endl;
    return false;
  }
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] =
    Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  if (!headerDone)
    return false;
  if (!readReply(uncomprSize, comprSize, compressor == COMPRESSORS::LZ4)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool FifoClient::readReply(size_t uncomprSize, size_t comprSize, bool bcompressed) {
  std::vector<char>& buffer = _memoryPool.getSecondaryBuffer(comprSize + 1);
  if (!Fifo::readString(_fdRead, buffer.data(), comprSize, _options._numberRepeatEINTR)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return printReply(buffer, uncomprSize, comprSize, bcompressed);
}

} // end of namespace fifo

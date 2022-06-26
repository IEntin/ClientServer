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

FifoClient::FifoClient(const ClientOptions& options) :
  Client(options), _fifoName(options._fifoName), _setPipeSize(options._setPipeSize) {}

FifoClient::~FifoClient() {
  CLOG << __FILE__ << ':' << __LINE__ << ' ' << __func__ << std::endl;
  try {
    Fifo::onExit(_fifoName, _options._numberRepeatENXIO, _options._ENXIOwait);
  }
  catch (...) {}
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
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__
	 << '-' << std::strerror(errno) << ' ' << _fifoName << '\n';
    return false;
  }
  if (_setPipeSize)
    Fifo::setPipeSize(_fdWrite, subtask.size());
  if (!Fifo::writeString(_fdWrite, std::string_view(subtask.data(), subtask.size()))) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
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
  auto [uncomprSize, comprSize, compressor, diagnostics, headerDone] =
    Fifo::readHeader(_fdRead, _options._numberRepeatEINTR);
  if (!headerDone)
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

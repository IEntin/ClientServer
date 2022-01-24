/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"
#include "CommUtility.h"
#include "Compression.h"
#include "MemoryPool.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>

namespace fifo {

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ':' << std::strerror(errno) << std::endl;
  _fd = -1;
}

const ssize_t Fifo::_defaultPipeSize = getDefaultPipeSize();

HEADER Fifo::readHeader(int fd) {
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE + 1] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ':' << std::strerror(errno) << std::endl;
	return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false, false);
      }
    }
    else if (result == 0) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
                << ":read(...) returns 0,EOF." << std::endl;
      return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false, false);
    }
    else
      readSoFar += result;
  }
  return utility::decodeHeader(std::string_view(buffer, HEADER_SIZE), readSoFar == HEADER_SIZE);
}

bool Fifo::sendReply(int fd, Batch& batch) {
  std::string_view sendView = commutility::buildReply(batch);
  if (sendView.empty())
    return false;
  if (fd == -1)
    return false;
  if (!writeString(fd, sendView)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
}

bool Fifo::writeString(int fd, std::string_view str) {
  size_t written = 0;
  while (written < str.size()) {
    ssize_t result = write(fd, str.data() + written, str.size() - written);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ' '
		  << strerror(errno) << ", written=" << written << " str.size()="
		  << str.size() << std::endl;
	return false;
      }
    }
    else
      written += result;
  }
  if (str.size() != written) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":str.size()="
	      << str.size() << "!=written=" << written << std::endl;
    return false;
  }
  return true;
}

bool Fifo::readString(int fd, char* received, size_t size) {
  size_t totalRead = 0;
  while (totalRead < size) {
    ssize_t result = read(fd, received + totalRead, size - totalRead);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		  << ':' << std::strerror(errno) << std::endl;
	return false;
      }
    }
    else if (result == 0) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-read(...) returns 0,EOF,another end closed connection." << std::endl;
      return false;
    }
    else
      totalRead += result;
  }
  if (totalRead != size) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__<< " size="
	      << size << " totalRead=" << totalRead << std::endl;
    return false;
  }
  return true;
}

bool Fifo::readBatch(int fd,
		     size_t uncomprSize,
		     size_t comprSize,
		     bool bcompressed,
		     std::ostream* pstream) {
  std::vector<char>& buffer = MemoryPool::getSecondaryBuffer(comprSize + 1);
  if (!readString(fd, buffer.data(), comprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  std::string_view received(buffer.data(), comprSize);
  std::ostream& stream = pstream ? *pstream : std::cout;
  if (bcompressed) {
    std::string_view dstView = Compression::uncompress(received, uncomprSize);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    stream << dstView; 
  }
  else
    stream << received;
  return true;
}

bool Fifo::readVectorChar(int fd,
			  size_t uncomprSize,
			  size_t comprSize,
			  bool bcompressed,
			  std::vector<char>& uncompressed) {
  if (bcompressed) {
    auto[buffer, bufferSize] = MemoryPool::getPrimaryBuffer(comprSize);
    if (!readString(fd, buffer, comprSize))
      return false;
    std::string_view received(buffer, comprSize);
    uncompressed.resize(uncomprSize);
    if (!Compression::uncompress(received, uncompressed)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
  }
  else {
    uncompressed.resize(uncomprSize);
    if (!readString(fd, uncompressed.data(), uncomprSize))
      return false;
  }
  return true;
}

bool Fifo::receive(int fd, std::vector<char>& uncompressed, HEADER& header) {
  header = readHeader(fd);
  const auto& [uncomprSize, comprSize, compressor, diagnostics, headerDone] = header;
  if (!headerDone)
    return false;
  return readVectorChar(fd, uncomprSize, comprSize, compressor == LZ4, uncompressed);
}

ssize_t Fifo::getDefaultPipeSize() {
  std::string_view testPipeName = "/tmp/testpipe";
  if (mkfifo(testPipeName.data(), 0620) == -1 && errno != EEXIST) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << '-' << testPipeName << std::endl;
    return -1;
  }
  int fd = open(testPipeName.data(), O_RDWR);
  if (fd == -1) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << '-' << testPipeName << std::endl;
    return -1;
  }
  ssize_t pipeSize = fcntl(fd, F_GETPIPE_SZ);
  if (pipeSize == -1) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << '-' << testPipeName << std::endl;
    return -1;
  }
  close(fd);
  return pipeSize;
}

} // end of namespace fifo

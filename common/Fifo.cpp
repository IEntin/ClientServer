#include "Fifo.h"
#include "Compression.h"
#include "MemoryPool.h"
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <poll.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

namespace fifo {

CloseFileDescriptor::CloseFileDescriptor(int fd) : _fd(fd) {}

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
	return std::make_tuple(-1, -1, 0, EMPTY_COMPRESSOR, false);
      }
    }
    else if (result == 0) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-read(...) return 0,readSoFar=" << readSoFar
		<< ",HEADER_SIZE=" << HEADER_SIZE << std::endl;
      return std::make_tuple(-1, -1, 0, EMPTY_COMPRESSOR, false);
    }
    else
      readSoFar += result;
  }
  return utility::decodeHeader(std::string_view(buffer, HEADER_SIZE), readSoFar == HEADER_SIZE);
}

ssize_t Fifo::writeString(int fd, std::string_view str) {
  static bool enableHints = ProgramOptions::get("EnableHints", true);
  if (enableHints)
    if (str.size() > static_cast<size_t>(_defaultPipeSize))
      static auto& dummy[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__
        << ' ' << __func__ << ":payload size=" << str.size() << " exceeds _defaultPipeSize="
        << _defaultPipeSize << ".For optimal performance this should not happen frequently."
        "The reason could be too large DYNAMIC_BUFFER_SIZE or disabled compression.\n"
        << "To disable this message set \"EnableHints\" to false." << std::endl;
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
	return -1;
      }
    }
    else
      written += result;
  }
  if (str.size() != written) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":str.size()="
	      << str.size() << "!=written=" << written << std::endl;
  }
  return written;
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
		<< "-read(...) return 0,totalRead=" << totalRead
		<< ",size=" << size << std::endl;
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
		     Batch& batch) {
  char* buffer = 0;
  std::vector<char> safetyBuffer;
  auto[ptr, bufferSize] = MemoryPool::getReceiveBuffer(comprSize);
  if (bufferSize >= comprSize)
    buffer = ptr;
  else {
    safetyBuffer.resize(comprSize + 1);
    buffer = &safetyBuffer[0];
  }
  if (!readString(fd, buffer, comprSize))
    return false;
  std::string_view received(buffer, comprSize);
  if (bcompressed) {
    std::string uncompressed; // may be unused if pooled buffer is big enough
    std::string_view uncompressedView = Compression::uncompress(received, uncomprSize, uncompressed);
    if (uncompressedView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to uncompress payload" << std::endl;
      return false;
    }
    utility::split(uncompressedView, batch); 
  }
  else
    utility::split(received, batch);
  return true;
}

bool Fifo::readVectorChar(int fd,
			  size_t uncomprSize,
			  size_t comprSize,
			  bool bcompressed,
			  std::vector<char>& uncompressed) {
  if (bcompressed) {
    char* buffer = 0;
    std::vector<char> safetyBuffer;
    auto[ptr, bufferSize] = MemoryPool::getBuffer(comprSize);
    if (bufferSize >= comprSize)
      buffer = ptr;
    else {
      safetyBuffer.resize(comprSize + 1);
      buffer = &safetyBuffer[0];
    }
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
    if (!readString(fd, &uncompressed[0], uncomprSize))
      return false;
  }
  return true;
}

bool Fifo::receive(int fd, Batch& batch) {
  unsigned count = 0;
  unsigned maxCount = 0;
  do {
    auto [uncomprSize, comprSize, strCount, compressor, headerDone] = readHeader(fd);
    if (!headerDone)
      return false;
    maxCount = strCount;
    if (!readBatch(fd, uncomprSize, comprSize, compressor == LZ4, batch))
      return false;
  } while(++count < maxCount);
  return true;
}

bool Fifo::receive(int fd, std::vector<char>& received) {
  unsigned count = 0;
  unsigned maxCount = 0;
  do {
    auto [uncomprSize, comprSize, strCount, compressor, headerDone] = readHeader(fd);
    if (!headerDone)
      return false;
    maxCount = strCount;
    std::vector<char> partialReceived;
    if (!readVectorChar(fd, uncomprSize, comprSize, compressor == LZ4, partialReceived))
      return false;
    received.insert(received.end(),
		    std::make_move_iterator(partialReceived.begin()),
		    std::make_move_iterator(partialReceived.end()));
  } while(++count < maxCount);
  return true;
}

bool Fifo::send(int fd, const Batch& batch) {
  for (std::string_view str : batch) {
    ssize_t result = writeString(fd, str);
    if (result == -1) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":error writeString" << std::endl;
      return false;
    }
  }
  return true;
}

ssize_t Fifo::getDefaultPipeSize() {
  std::string_view testPipeName = "/tmp/testpipe";
  if (mkfifo(testPipeName.data(), 0620) == -1 && errno != EEXIST) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << '-'
	      << std::strerror(errno) << '-' << testPipeName << std::endl;
    return -1;
  }
  int fd = open(testPipeName.data(), O_RDONLY | O_NONBLOCK);
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"
#include "Logger.h"
#include "Options.h"
#include "Utility.h"
#include <chrono>
#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <thread>
#include <unistd.h>

namespace fifo {

bool Fifo::readMsgNonBlock(std::string_view name,
			   HEADER& header,
			   std::vector<char>& body,
			   const Options& options) {
  int fdRead = -1;
  utility::CloseFileDescriptor cfdr(fdRead);
  fdRead = openReadNonBlock(name, fdRead);
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  fdWrite = openWriteNonBlock(name, options);
  if (pollFd(fdRead, POLLIN) == -1)
    return false;
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fdRead, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	if (pollFd(fdRead, POLLIN) == -1)
	  throw std::runtime_error(std::strerror(errno));
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      fdRead = openReadNonBlock(name, fdRead);
      if (pollFd(fdRead, POLLIN) == -1)
	throw std::runtime_error(std::strerror(errno));
      continue;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != HEADER_SIZE) {
    LogError << "HEADER_SIZE=" << HEADER_SIZE
	     << " readSoFar=" << readSoFar << std::endl;
    throw std::runtime_error(std::strerror(errno));
  }
  header = decodeHeader(buffer);
  if (!isOk(header)) {
    LogError << "header is invalid." << std::endl;
    return false;
  }
  size_t comprSize = extractCompressedSize(header);
  body.resize(comprSize);
  return readString(fdRead, body.data(), comprSize);
}

bool Fifo::readMsgBlock(std::string_view name,
			HEADER& header,
			std::vector<char>& body) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  if (fd == -1) {
    LogError << name << '-' << std::strerror(errno) << std::endl;
    return false;
  }
  if (pollFd(fd, POLLIN) == -1)
    return false;
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	if (pollFd(fd, POLLIN) == -1)
	  throw std::runtime_error(std::strerror(errno));
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      Debug << (errno ? std::strerror(errno) : "EOF") << std::endl;
      return false;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != HEADER_SIZE) {
    LogError << "HEADER_SIZE=" << HEADER_SIZE
	     << " readSoFar=" << readSoFar << std::endl;
    throw std::runtime_error(std::strerror(errno));
  }
  header = decodeHeader(buffer);
  if (!isOk(header)) {
    LogError << "header is invalid." << std::endl;
    return false;
  }
  size_t comprSize = extractCompressedSize(header);
  body.resize(comprSize);
  return readString(fd, body.data(), comprSize);
}

bool Fifo::readString(int fd, char* received, size_t size) {
  size_t readSoFar = 0;
  while (readSoFar < size) {
    ssize_t result = read(fd, received + readSoFar, size - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	if (pollFd(fd, POLLIN) == -1)
	  return false;
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	return false;
      }
    }
    else if (result == 0) {
      Info << (errno ? std::strerror(errno) : "EOF") << std::endl;
      return false;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != size) {
    LogError << "size=" << size << " readSoFar=" << readSoFar << std::endl;
    return false;
  }
  return true;
}

bool Fifo::writeString(int fd, std::string_view str) {
  if (pollFd(fd, POLLOUT) == -1)
    return false;
  size_t written = 0;
  while (written < str.size()) {
    ssize_t result = write(fd, str.data() + written, str.size() - written);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	LogError << strerror(errno) << ", written=" << written
		 << " str.size()=" << str.size() << std::endl;
	return false;
      }
    }
    else
      written += static_cast<size_t>(result);
  }
  if (str.size() != written) {
    LogError << "str.size()=" << str.size()
	     << "!=written=" << written << std::endl;
    return false;
  }
  return true;
}

bool Fifo::sendMsg(int fd, const HEADER& header, std::string_view body) {
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  return writeString(fd, std::string_view(buffer, HEADER_SIZE)) &&
    writeString(fd, std::string_view(body.data(), body.size()));
}

short Fifo::pollFd(int& fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  pfd.revents = 0;
  int result = poll(&pfd, 1, -1);
  if (result <= 0) {
    LogError << strerror(errno) << std::endl;
    return result;
  }
  else if (pfd.revents & POLLERR) {
    Info << std::strerror(errno) << std::endl;
    return POLLERR;
  }
  else if (pfd.revents & POLLHUP) {
    Debug << std::strerror(errno) << std::endl;
    return POLLHUP;
  }
  else if (pfd.revents & POLLNVAL) {
    Debug << std::strerror(errno) << std::endl;
    return POLLNVAL;
  }
  else if (pfd.revents & expected)
    return expected;
  else
    return -1;
}

bool Fifo::setPipeSize(int fd, long requested) {
  long currentSz = fcntl(fd, F_GETPIPE_SZ);
  if (currentSz == -1) {
    LogError << std::strerror(errno) << std::endl;
    return false;
  }
  if (requested > currentSz) {
    int ret = fcntl(fd, F_SETPIPE_SZ, requested);
    if (ret < 0) {
      static auto& printOnce[[maybe_unused]] =
	Info << std::strerror(errno) << ":\n"
	     << "su privileges required, ignore." << std::endl;
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      Info << std::strerror(errno) << std::endl;
      return false;
    }
    return newSz >= requested || requested < currentSz;
  }
  return false;
}

// unblock the call to blocking open calls by opening opposite end.
void Fifo::onExit(std::string_view fifoName, const Options& options) {
  int fdRead = -1;
  utility::CloseFileDescriptor cfdr(fdRead);
  fdRead = openReadNonBlock(fifoName, fdRead);
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  fdWrite = openWriteNonBlock(fifoName, options);
}

int Fifo::openWriteNonBlock(std::string_view fifoName, const Options& options) {
  int fd = -1;
  int rep = 0;
  do {
    close(fd);
    fd = -1;
    fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      switch (errno) {
      case ENXIO:
      case EINTR:
	std::this_thread::sleep_for(std::chrono::milliseconds(options._ENXIOwait));
	break;
      default:
	return fd;
	break;
      }
    }
  } while (fd == -1 && rep++ < options._numberRepeatENXIO);
  if (fd >= 0 && options._setPipeSize)
    setPipeSize(fd, options._pipeSize);
  return fd;
}

int Fifo::openReadNonBlock(std::string_view fifoName, int& fd) {
  int fdOld = fd;
  fd = open(fifoName.data(), O_RDONLY | O_NONBLOCK);
  close(fdOld);
  if (fd == -1)
    Info << std::strerror(errno) << std::endl;
  return fd;
}

} // end of namespace fifo

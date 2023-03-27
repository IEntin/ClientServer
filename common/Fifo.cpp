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

// non blocking read
HEADER Fifo::readHeader(std::string_view name, int& fd) {
  fd = openReadNonBlock(name, fd);
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	short event = -1;
	do {
	  event = Fifo::pollFd(fd, POLLIN);
	  if (event == -1)
	    throw std::runtime_error(std::strerror(errno));
	} while (event != POLLIN);
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      int event = -1;
      do {
	fd = openReadNonBlock(name, fd);
	event = Fifo::pollFd(fd, POLLIN);
	if (event == -1)
	  throw std::runtime_error(std::strerror(errno));
      } while (event != POLLIN);
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
  return decodeHeader(buffer);
}

// non blocking read
bool Fifo::readMsgNonBlock(std::string_view name,
			   int& fd,
			   HEADER& header,
			   std::vector<char>& body) {
  fd = openReadNonBlock(name, fd);
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	short event = -1;
	do {
	  event = Fifo::pollFd(fd, POLLIN);
	  if (event == -1)
	    throw std::runtime_error(std::strerror(errno));
	} while (event != POLLIN);
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      short event = -1;
      do {
	fd = openReadNonBlock(name, fd);
	event = Fifo::pollFd(fd, POLLIN);
	if (event == -1)
	  throw std::runtime_error(std::strerror(errno));
      } while (event != POLLIN);
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
  if (!isOk(header))
    return false;
  size_t comprSize = extractCompressedSize(header);
  body.resize(comprSize);
  return readString(fd, body.data(), comprSize);
}

HEADER Fifo::readHeader(int fd) {
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      short event = -1;
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	do {
	  event = pollFd(fd, POLLIN);
	  if (event == -1)
	    throw std::runtime_error(std::strerror(errno));
	} while (event != POLLIN);
	continue;
      }
      else {
	LogError << std::strerror(errno) << std::endl;
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      Info << (errno ? std::strerror(errno) : "EOF") << std::endl;
      return { HEADERTYPE::ERROR, 0, 0, COMPRESSORS::NONE, false, STATUS::FIFO_PROBLEM };
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != HEADER_SIZE) {
    LogError << "HEADER_SIZE=" << HEADER_SIZE
	     << " readSoFar=" << readSoFar << std::endl;
    throw std::runtime_error(std::strerror(errno));
  }
  return decodeHeader(buffer);
}

bool Fifo::readString(int fd, char* received, size_t size) {
  size_t readSoFar = 0;
  while (readSoFar < size) {
    ssize_t result = read(fd, received + readSoFar, size - readSoFar);
    if (result == -1) {
      // non blocking read
      short event = -1;
      if (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK) {
	do {
	  event = pollFd(fd, POLLIN);
	  if (event == -1)
	    return false;
	} while (event != POLLIN);
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
  do {
    pfd.revents = 0;
    int result = poll(&pfd, 1, -1);
    if (result <= 0) {
      LogError << strerror(errno) << std::endl;
      return 0;
    }
    else if (pfd.revents & POLLERR) {
      LogError << std::strerror(errno) << std::endl;
      return -1;
    }
    else if (pfd.revents & POLLHUP) {
      LogError << std::strerror(errno) << std::endl;
      return -1;
    }
    else if (pfd.revents & expected)
      return expected;
    else
      return -1;
  } while (errno == EINTR);
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
	LogError << std::strerror(errno) << ":\n"
		 << "su privileges required, ignore." << std::endl;
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      LogError << std::strerror(errno) << std::endl;
      return false;
    }
    return newSz >= requested || requested < currentSz;
  }
  return false;
}

// unblock the call to blocking open calls by opening opposite end.
void Fifo::onExit(std::string_view fifoName, const Options& options) {
  int fdWrite = -1;
  utility::CloseFileDescriptor cfdw(fdWrite);
  fdWrite = openWriteNonBlock(fifoName, options);
  int fdRead = -1;
  utility::CloseFileDescriptor cfdr(fdRead);
  fdRead = openReadNonBlock(fifoName, fdRead);
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"
#include <chrono>
#include <filesystem>
#include <poll.h>
#include <thread>

namespace fifo {

bool Fifo::readMsgNonBlock(const Options& options,
			   std::string_view name,
			   HEADER& header,
			   std::vector<char>& body) {
  int fdRead = openReadNonBlock(options, name);
  utility::CloseFileDescriptor cfdr(fdRead);
  size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fdRead, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
	continue;
      }
      else {
	LogError << std::strerror(errno) << '\n';
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      int fdOld = fdRead;
      fdRead = openReadNonBlock(options, name);
      if (fdOld != -1)
	close(fdOld);
      continue;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != HEADER_SIZE) {
    LogError << "HEADER_SIZE=" << HEADER_SIZE
	     << " readSoFar=" << readSoFar << '\n';
    throw std::runtime_error(std::strerror(errno));
  }
  header = decodeHeader(buffer);
  if (!isOk(header)) {
    LogError << "header is invalid." << '\n';
    return false;
  }
  size_t payloadSize = extractPayloadSize(header);
  body.resize(payloadSize);
  return readString(fdRead, body.data(), payloadSize);
}

bool Fifo::readString(int fd, char* received, size_t size) {
  size_t readSoFar = 0;
  while (readSoFar < size) {
    ssize_t result = read(fd, received + readSoFar, size - readSoFar);
    if (result == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK)
	continue;
      else {
	LogError << std::strerror(errno) << '\n';
	return false;
      }
    }
    else if (result == 0) {
      Info << (errno ? std::strerror(errno) : "EOF") << '\n';
      return false;
    }
    else
      readSoFar += static_cast<size_t>(result);
  }
  if (readSoFar != size) {
    LogError << "size=" << size << " readSoFar=" << readSoFar << '\n';
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
		 << " str.size()=" << str.size() << '\n';
	return false;
      }
    }
    else
      written += static_cast<size_t>(result);
  }
  if (str.size() != written) {
    LogError << "str.size()=" << str.size()
	     << "!=written=" << written << '\n';
    return false;
  }
  return true;
}

bool Fifo::sendMsg(std::string_view name,
		   const HEADER& header,
		   const Options& options,
		   std::string_view body) {
  int fdWrite = openWriteNonBlock(name, options);
  utility::CloseFileDescriptor cfdw(fdWrite);
  if (fdWrite == -1)
    return false;
  if (options._setPipeSize)
    setPipeSize(fdWrite, options._pipeSize);
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  return writeString(fdWrite, std::string_view(buffer, HEADER_SIZE)) &&
    writeString(fdWrite, std::string_view(body.data(), body.size()));
}

short Fifo::pollFd(int fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  pfd.revents = 0;
  int result = poll(&pfd, 1, -1);
  if (result <= 0) {
    LogError << strerror(errno) << '\n';
    return result;
  }
  else if (pfd.revents & POLLERR) {
    Info << "errno=" << errno << ' ' << std::strerror(errno) << '\n';
    return -1;
  }
  else if (pfd.revents & POLLHUP) {
    Debug << std::strerror(errno) << '\n';
    return POLLHUP;
  }
  else if (pfd.revents & POLLNVAL) {
    Debug << std::strerror(errno) << '\n';
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
    LogError << std::strerror(errno) << '\n';
    return false;
  }
  if (requested > currentSz) {
    int ret = fcntl(fd, F_SETPIPE_SZ, requested);
    if (ret < 0) {
      static thread_local auto& printOnce[[maybe_unused]] =
	Info << std::strerror(errno) << ":\n"
	     << "su privileges required, ignore." << '\n';
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      Info << std::strerror(errno) << '\n';
      return false;
    }
    return newSz >= requested || requested < currentSz;
  }
  return false;
}

// unblock calls to blocking open read by opening opposite end.
void Fifo::onExit(std::string_view fifoName, const Options& options) {
  int fdWrite = openWriteNonBlock(fifoName, options);
  utility::CloseFileDescriptor cfdw(fdWrite);
}

int Fifo::openWriteNonBlock(std::string_view fifoName, const Options& options) {
  int fd = -1;
  int rep = 0;
  do {
    if (fd != -1)
      close(fd);
    fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      switch (errno) {
      case ENXIO:
	std::this_thread::sleep_for(std::chrono::milliseconds(options._ENXIOwait));
	break;
      default:
	return fd;
	break;
      }
    }
  } while (fd == -1 && rep++ < options._numberRepeatENXIO);
  if (fd != -1 && options._setPipeSize)
    setPipeSize(fd, options._pipeSize);
  return fd;
}

int Fifo::openReadNonBlock(const Options& options, std::string_view fifoName) {
  if (!std::filesystem::exists(fifoName))
    return -1;
  int fd = open(fifoName.data(), O_RDONLY | O_NONBLOCK);
  if (fd == -1)
    Info << std::strerror(errno) << ' ' << fifoName << '\n';
  if (options._setPipeSize)
    setPipeSize(fd, options._pipeSize);
  return fd;
}

} // end of namespace fifo

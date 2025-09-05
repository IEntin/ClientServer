/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"

#include <cstring>
#include <poll.h>
#include <thread>

#include "IOUtility.h"
#include "Options.h"

namespace fifo {

static constexpr std::size_t BUFFER_SIZE = 10000;

thread_local std::string Fifo::_payload;
std::string Fifo::_emptyString;

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    LogError << strerror(errno) << '\n';
  _fd = -1;
}

short Fifo::pollFd(int fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  int result = poll(&pfd, 1, -1);
  if (result <= 0 || pfd.revents & POLLERR || pfd.revents & POLLNVAL)
    throw std::runtime_error(ioutility::createErrorString());
  else if (pfd.revents & expected)
    return expected;
  else if (pfd.revents & POLLHUP) {
    Debug << strerror(errno) << '\n';
    return POLLHUP;
  }
  return -1;
}

bool Fifo::setPipeSize(int fd) {
  if (!Options::_setPipeSize)
    return false;
  ssize_t currentSz = fcntl(fd, F_GETPIPE_SZ);
  if (currentSz == -1)
    throw std::runtime_error(ioutility::createErrorString());
  unsigned long currentSize = std::bit_cast<unsigned long>(currentSz);
  if (Options::_pipeSize > currentSize) {
    ssize_t ret = fcntl(fd, F_SETPIPE_SZ, Options::_pipeSize);
    if (ret == -1) {
      static auto& printOnce[[maybe_unused]] =
	Info << strerror(errno) << ":\n"
	     << "su privileges required, ignore." << '\n';
      return false;
    }
    ssize_t newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      Info << strerror(errno) << '\n';
      return false;
    }
    unsigned long newSize = std::bit_cast<unsigned long>(newSz);
    return newSize >= Options::_pipeSize || Options::_pipeSize < currentSize;
  }
  return false;
}

// unblock calls to blocking open read by opening opposite end.
void Fifo::onExit(std::string_view fifoName) {
  int fdWrite = openWriteNonBlock(fifoName);
  CloseFileDescriptor cfdw(fdWrite);
}

int Fifo::openWriteNonBlock(std::string_view fifoName) {
  int fd = -1;
  int rep = 0;
  do {
    fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      // wait until read end is open
      switch (errno) {
      case ENXIO:
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	break;
      default:
	LogError << strerror(errno) << '\n';
	return fd;
      }
    }
  } while (fd == -1 && rep++ < Options::_numberRepeatENXIO);
  if (fd >= 0)
    setPipeSize(fd);
  return fd;
}

int Fifo::openReadNonBlock(std::string_view fifoName) {
  return open(fifoName.data(), O_RDONLY | O_NONBLOCK);
}

bool Fifo::readStringBlock(std::string_view name, std::string& payload) {
  int fd = open(name.data(), O_RDONLY);
  if (fd == -1)
    return false;
  CloseFileDescriptor cfdr(fd);
  std::size_t accumulatedSz = 0;
  char buffer[BUFFER_SIZE] = {};
  while (true) {
    ssize_t result = read(fd, buffer, BUFFER_SIZE);
    if (result == -1)
      throw std::runtime_error(ioutility::createErrorString());
    else if (result == 0)
      return false;
    else if (result > 0) {
      std::size_t transferred = std::bit_cast<std::size_t>(result);
      std::size_t prevSize = accumulatedSz;
      accumulatedSz += transferred;
      payload.resize(accumulatedSz);
      std::copy(std::cbegin(buffer), std::cbegin(buffer) + transferred, payload.begin() + prevSize);
      if (payload.ends_with(ENDOFMESSAGE)) {
	payload.erase(payload.size() - ENDOFMESSAGESZ);
	return true;
      }
    }
  }
  return false;
}

bool Fifo::readStringNonBlock(std::string_view name, std::string& payload) {
  int fd = openReadNonBlock(name);
  if (fd == -1)
    return false;
  CloseFileDescriptor cfdr(fd);
  std::size_t accumulatedSz = 0;
  while (true) {
    char buffer[BUFFER_SIZE] = {};
    ssize_t result = read(fd, buffer, BUFFER_SIZE);
    if (result == -1) {
      switch (errno) {
      case EAGAIN: {
	int ret =  pollFd(fd, POLLIN);
	if (ret != POLLIN)
	  return false;
	continue;
      }
	break;
      default:
	throw std::runtime_error(ioutility::createErrorString());
      }
    }
    else if (result > 0) {
      std::size_t transferred = std::bit_cast<std::size_t>(result);
      std::size_t prevSize = accumulatedSz;
      accumulatedSz += transferred;
      payload.resize(accumulatedSz);
      std::copy(std::cbegin(buffer), std::cbegin(buffer) + transferred, payload.begin() + prevSize);
      if (payload.ends_with(ENDOFMESSAGE)) {
	payload.erase(payload.size() - ENDOFMESSAGESZ);
	return true;
      }
   }
  }
  return false;
}

bool Fifo::readMessage(std::string_view name, bool block, std::string& payload) {
  if (block) {
    if (!readStringBlock(name, payload))
      return false;
  }
  else {
    if (!readStringNonBlock(name, payload))
      return false;
  }
  return true;
}

bool Fifo::readMessage(std::string_view name,
		       bool block,
		       HEADER& header,
		       std::span<std::reference_wrapper<std::string>> array) {
  _payload.clear();
  if (!readMessage(name, block, _payload))
    return false;
  return ioutility::processMessage(_payload, header, array);
}

bool Fifo::writeString(int fd, const char* str, std::size_t size) {
  std::size_t written = 0;
  while (written < size) {
    ssize_t result = write(fd, str + written, size - written);
    if (result == -1) {
      switch (errno) {
      case EAGAIN: {
	int ret = pollFd(fd, POLLOUT);
	if (ret != POLLOUT)
	  return false;
	continue;
      }
      default:
	throw std::runtime_error(ioutility::createErrorString());
      }
    }
    else
      written += result;
  }
  return true;
}

} // end of namespace fifo

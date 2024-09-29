/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"

#include <sys/ioctl.h>
#include <thread>

#include "ClientOptions.h"
#include "ServerOptions.h"

namespace fifo {

short Fifo::pollFd(int fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  int result = poll(&pfd, 1, -1);
  if (result <= 0 || pfd.revents & POLLERR || pfd.revents & POLLNVAL)
    throw std::runtime_error(utility::createErrorString());
  else if (pfd.revents & expected)
    return expected;
  else if (pfd.revents & POLLHUP) {
    Debug << strerror(errno) << '\n';
    return POLLHUP;
  }
  return -1;
}

bool Fifo::setPipeSize(int fd) {
  bool setPipeBufferSize = false;
  long pipeSize = 0;
  if (ServerOptions::_parsed) {
    setPipeBufferSize = ServerOptions::_setPipeSize;
    pipeSize = ServerOptions::_pipeSize;
  }
  else if (ClientOptions::_parsed) {
    setPipeBufferSize = ClientOptions::_setPipeSize;
    pipeSize = ClientOptions::_pipeSize;
  }
  if (!setPipeBufferSize)
    return false;
  long currentSz = fcntl(fd, F_GETPIPE_SZ);
  if (currentSz == -1)
    throw std::runtime_error(utility::createErrorString());
  if (pipeSize > currentSz) {
    int ret = fcntl(fd, F_SETPIPE_SZ, pipeSize);
    if (ret < 0) {
      static auto& printOnce[[maybe_unused]] =
	Info << strerror(errno) << ":\n"
	     << "su privileges required, ignore." << '\n';
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      Info << strerror(errno) << '\n';
      return false;
    }
    return newSz >= pipeSize || pipeSize < currentSz;
  }
  return false;
}

// unblock calls to blocking open read by opening opposite end.
void Fifo::onExit(std::string_view fifoName) {
  int fdWrite = openWriteNonBlock(fifoName);
  utility::CloseFileDescriptor cfdw(fdWrite);
}

int Fifo::openWriteNonBlock(std::string_view fifoName) {
  int numberRepeatENXIO = 0;
  if (ServerOptions::_parsed)
    numberRepeatENXIO = ServerOptions::_numberRepeatENXIO;
  else if (ClientOptions::_parsed)
    numberRepeatENXIO = ClientOptions::_numberRepeatENXIO;
  int fd = -1;
  int rep = 0;
  do {
    fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      // wait until read end is open
      switch (errno) {
      case ENOENT:
      case ENXIO:
	std::this_thread::sleep_for(std::chrono::milliseconds(1));
	break;
      default:
	return fd;
      }
    }
  } while (fd == -1 && rep++ < numberRepeatENXIO);
  if (fd != -1)
    setPipeSize(fd);
  return fd;
}

int Fifo::openReadNonBlock(std::string_view fifoName) {
  return open(fifoName.data(), O_RDONLY | O_NONBLOCK);
}

bool Fifo::sendMsgNonBlock(std::string_view name, std::string_view payload) {
  int fdWrite = openWriteNonBlock(name);
  utility::CloseFileDescriptor cfdw(fdWrite);
  if (fdWrite == -1)
    return false;
  writeString(fdWrite, payload);
  writeString(fdWrite, utility::ENDOFMESSAGE);
  return true;
}

bool Fifo::readStringBlock(std::string_view name, std::string& payload) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  if (fd == -1)
    return false;
  static constexpr std::size_t BUFFER_SIZE = 10000;
  char buffer[BUFFER_SIZE] = {};
  while (true) {
    ssize_t result = read(fd, buffer, BUFFER_SIZE);
    if (result == -1)
      throw std::runtime_error(utility::createErrorString());
    else if (result == 0) {
      if (payload.ends_with(utility::ENDOFMESSAGE))
	payload.erase(payload.size() - utility::ENDOFMESSAGE.size());
      break;
    }
    else {
      payload.append(buffer, result);
      if (payload.ends_with(utility::ENDOFMESSAGE))
	payload.erase(payload.size() - utility::ENDOFMESSAGE.size());
    }
  }
  return !payload.empty();
}

bool Fifo::readStringNonBlock(std::string_view name, std::string& payload) {
  int fd = openReadNonBlock(name);
  if (fd == -1)
    return false;
  utility::CloseFileDescriptor cfdr(fd);
  while (true) {
    static constexpr std::size_t BUFFER_SIZE = 10000;
    char buffer[BUFFER_SIZE] = {};
    ssize_t result = read(fd, buffer, BUFFER_SIZE);
    if (result == -1) {
      switch (errno) {
      case EAGAIN:
	continue;
	break;
      default:
	throw std::runtime_error(utility::createErrorString());
	break;
      }
    }
    else if (result >= 0) {
      if (result > 0)
	payload.append(buffer, result);
      if (payload.ends_with(utility::ENDOFMESSAGE)) {
	payload.erase(payload.size() - utility::ENDOFMESSAGE.size());
	break;
      }
    }
  }
  return !payload.empty();
}

} // end of namespace fifo

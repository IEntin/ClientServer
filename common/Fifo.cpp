/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"

#include <thread>

#include "ClientOptions.h"
#include "IOUtility.h"
#include "ServerOptions.h"

namespace fifo {
bool Fifo::readMsgNonBlock(int fdRead, std::string& payload) {
  char buffer[ioutility::CONV_BUFFER_SIZE] = {};
  readString(fdRead, buffer, ioutility::CONV_BUFFER_SIZE);
  std::size_t payloadSize = 0;
  ioutility::fromChars(buffer, payloadSize);
  payload.resize(payloadSize);
  readString(fdRead, payload.data(), payloadSize);
  return true;
}

void Fifo::readMsgBlock(std::string_view name, std::string& payload) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  char buffer[ioutility::CONV_BUFFER_SIZE] = {};
  readString(fd, buffer, ioutility::CONV_BUFFER_SIZE);
  std::size_t payloadSize = 0;
  ioutility::fromChars(buffer, payloadSize);
  payload.resize(payloadSize);
  readString(fd, payload.data(), payloadSize);
}

bool Fifo::sendMsg(std::string_view name, std::string_view payload) {
  int fdWrite = openWriteNonBlock(name);
  utility::CloseFileDescriptor cfdw(fdWrite);
  if (fdWrite == -1)
    return false;
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payload.size(), sizeStr);
  std::string_view view(sizeStr, ioutility::CONV_BUFFER_SIZE);
  writeString(fdWrite, view);
  writeString(fdWrite, payload);
  return true;
}

bool Fifo::sendMsg(int fdWrite, std::string_view payload) {
  char sizeStr[ioutility::CONV_BUFFER_SIZE] = {};
  ioutility::toChars(payload.size(), sizeStr);
  std::string_view view(sizeStr, ioutility::CONV_BUFFER_SIZE);
  writeString(fdWrite, view);
  writeString(fdWrite, payload);
  return true;
}

short Fifo::pollFd(int fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  int result = poll(&pfd, 1, -1);
  if (result <= 0 || pfd.revents & POLLERR || pfd.revents & POLLNVAL)
    throw std::runtime_error(utility::createErrorString());
  else if (pfd.revents & expected)
    return expected;
  else if (pfd.revents & POLLHUP) {
    Debug << std::strerror(errno) << '\n';
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
	Info << std::strerror(errno) << ":\n"
	     << "su privileges required, ignore." << '\n';
      return false;
    }
    long newSz = fcntl(fd, F_GETPIPE_SZ);
    if (newSz == -1) {
      Info << std::strerror(errno) << '\n';
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

int Fifo::openWriteNonBlockOpenedRead(std::string_view fifoName) {
  int fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
  if (fd != -1)
    setPipeSize(fd);
  return fd;
}

int Fifo::openReadNonBlock(std::string_view fifoName) {
  return open(fifoName.data(), O_RDONLY | O_NONBLOCK);
}

} // end of namespace fifo

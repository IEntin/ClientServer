/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Fifo.h"

#include <chrono>
#include <fcntl.h>
#include <filesystem>
#include <poll.h>
#include <thread>

#include "ClientOptions.h"
#include "Logger.h"
#include "ServerOptions.h"

namespace fifo {

bool Fifo::readMsgNonBlock(std::string_view name, HEADER& header, std::string& body) {
  int fdRead = openReadNonBlock(name);
  if (fdRead == -1)
    return false;
  utility::CloseFileDescriptor cfdr(fdRead);
  char buffer[HEADER_SIZE] = {};
  readString(fdRead, buffer, HEADER_SIZE);
  header = decodeHeader(buffer);
  std::size_t payloadSize = extractPayloadSize(header);
  body.resize(payloadSize);
  readString(fdRead, body.data(), payloadSize);
  return true;
}

bool Fifo::readMsgNonBlock(std::string_view name, std::string& payload) {
  int fdRead = openReadNonBlock(name);
  if (fdRead == -1)
    return false;
  utility::CloseFileDescriptor cfdr(fdRead);
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

bool Fifo::readMsgBlock(std::string_view name, HEADER& header, std::string& body) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  std::size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno != EAGAIN)
	throw std::runtime_error(std::strerror(errno));
    }
    else if (result == 0) {
      Info << name << "-EOF" << '\n';
      return false;
    }
    else
      readSoFar += result;
  }
  header = decodeHeader(buffer);
  std::size_t payloadSize = extractPayloadSize(header);
  body.resize(payloadSize);
  readString(fd, body.data(), payloadSize);
  return true;
}

bool Fifo::readMsgBlock(std::string_view name,
			HEADER& header,
			std::string& payload1,
			CryptoPP::SecByteBlock& payload2) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  std::size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno != EAGAIN)
	throw std::runtime_error(std::strerror(errno));
    }
    else if (result == 0) {
      Info << name << "-EOF" << '\n';
      return false;
    }
    else
      readSoFar += result;
  }
  header = decodeHeader(buffer);
  std::size_t payload1Size = extractPayloadSize(header);
  payload1.resize(payload1Size);
  readString(fd, payload1.data(), payload1Size);
  std::size_t payload2Size = extractParameter(header);
  payload2.resize(payload2Size);
  readString(fd, payload2.data(), payload2Size);
  return true;
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

bool Fifo::sendMsg(std::string_view name,
		   const HEADER& header,
		   std::string_view payload1,
		   CryptoPP::SecByteBlock& payload2) {
  int fdWrite = openWriteNonBlock(name);
  utility::CloseFileDescriptor cfdw(fdWrite);
  if (fdWrite == -1)
    return false;
  char buffer[HEADER_SIZE] = {};
  encodeHeader(buffer, header);
  std::string_view headerView(buffer, HEADER_SIZE);
  writeString(fdWrite, headerView);
  writeString(fdWrite, payload1);
  writeString(fdWrite, payload2);
  return true;
}

short Fifo::pollFd(int fd, short expected) {
  pollfd pfd{ fd, expected, 0 };
  pfd.revents = 0;
  int result = poll(&pfd, 1, -1);
  if (result <= 0)
    throw std::runtime_error(std::strerror(errno));
  else if (pfd.revents & POLLERR) {
    Info << std::strerror(errno) << '\n';
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
    throw std::runtime_error(std::strerror(errno));
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
  int ENXIOwait = 0;
  if (ServerOptions::_parsed) {
    numberRepeatENXIO = ServerOptions::_numberRepeatENXIO;
    ENXIOwait = ServerOptions::_ENXIOwait;
  }
  else if (ClientOptions::_parsed) {
    numberRepeatENXIO = ClientOptions::_numberRepeatENXIO;
    ENXIOwait = ClientOptions::_ENXIOwait;
  }
  int fd = -1;
  int rep = 0;
  do {
    fd = open(fifoName.data(), O_WRONLY | O_NONBLOCK);
    if (fd == -1) {
      switch (errno) {
      case ENOENT:
      case ENXIO:
	std::this_thread::sleep_for(std::chrono::milliseconds(ENXIOwait));
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
  if (!std::filesystem::exists(fifoName))
    return -1;
  int fd = open(fifoName.data(), O_RDONLY | O_NONBLOCK);
  if (fd != -1)
    Warn << std::strerror(errno) << ' ' << fifoName << '\n';
  return fd;
}

} // end of namespace fifo

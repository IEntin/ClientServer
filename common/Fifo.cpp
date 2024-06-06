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
#include "Utility.h"

namespace fifo {

bool Fifo::readMsgNonBlock(std::string_view name, HEADER& header, std::string& body) {
  int fdRead = openReadNonBlock(name);
  utility::CloseFileDescriptor cfdr(fdRead);
  char buffer[HEADER_SIZE] = {};
  if (!readString(fdRead, buffer, HEADER_SIZE)) {
    throw std::runtime_error(std::strerror(errno));
  }
  header = decodeHeader(buffer);
  if (!isOk(header)) {
    LogError << "header is invalid." << '\n';
    return false;
  }
  std::size_t payloadSize = extractPayloadSize(header);
  body.resize(payloadSize);
  return readString(fdRead, body.data(), payloadSize);
}

bool Fifo::readMsgBlock(std::string_view name, HEADER& header, std::string& body) {
  int fd = open(name.data(), O_RDONLY);
  utility::CloseFileDescriptor cfdr(fd);
  if (fd == -1) {
    LogError << name << '-' << std::strerror(errno) << '\n';
    return false;
  }
  std::size_t readSoFar = 0;
  char buffer[HEADER_SIZE] = {};
  while (readSoFar < HEADER_SIZE) {
    ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
    if (result == -1) {
      if (errno != EAGAIN) {
	LogError << std::strerror(errno) << '\n';
	throw std::runtime_error(std::strerror(errno));
      }
    }
    else if (result == 0) {
      Warn << (errno ? std::strerror(errno) : "EOF") << '-' << name << '\n';
      return false;
    }
    else
      readSoFar += result;
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
  std::size_t payloadSize = extractPayloadSize(header);
  body.resize(payloadSize);
  return readString(fd, body.data(), payloadSize);
}

bool Fifo::readString(int fd, char* received, std::size_t size) {
  std::size_t readSoFar = 0;
  while (readSoFar < size) {
    ssize_t result = read(fd, received + readSoFar, size - readSoFar);
    if (result == -1) {
      if (errno != EAGAIN) {
	LogError << std::strerror(errno) << '\n';
	return false;
      }
    }
    else
      readSoFar += result;
  }
  if (readSoFar != size) {
    LogError << "size=" << size << " readSoFar=" << readSoFar << '\n';
    return false;
  }
  return true;
}

bool Fifo::writeString(int fd, std::string_view str) {
  std::size_t written = 0;
  while (written < str.size()) {
    ssize_t result = write(fd, str.data() + written, str.size() - written);
    if (result == -1) {
      if (errno == EAGAIN)
	continue;
      else {
	LogError << strerror(errno) << ", written=" << written
		 << " str.size()=" << str.size() << '\n';
	return false;
      }
    }
    else
      written += result;
  }
  if (str.size() != written) {
    LogError << "str.size()=" << str.size()
	     << "!=written=" << written << '\n';
    return false;
  }
  return true;
}

bool Fifo::sendMsg(std::string_view name, const HEADER& header, std::string_view body) {
  int fdWrite = openWriteNonBlock(name);
  utility::CloseFileDescriptor cfdw(fdWrite);
  if (fdWrite == -1)
    return false;
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
  if (currentSz == -1) {
    LogError << std::strerror(errno) << '\n';
    return false;
  }
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
    Info << std::strerror(errno) << ' ' << fifoName << '\n';
  return fd;
}

} // end of namespace fifo

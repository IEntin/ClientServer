/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>
#include <poll.h>

#include "Logger.h"
#include "Utility.h"

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int fd, short expected);

public:
  template <typename P1, typename P2 = P1>
  static bool readMsg(std::string_view name,
		      bool nonblock,
		      HEADER& header,
		      P1& payload1,
		      P2&& payload2 = P2()) {
    int fd = -1;
    if (nonblock)
      fd = openReadNonBlock(name);
    else
      fd = open(name.data(), O_RDONLY);
    utility::CloseFileDescriptor cfdr(fd);
    if (pollFd(fd, POLLIN) != POLLIN)
      return false;
    std::size_t readSoFar = 0;
    char buffer[HEADER_SIZE] = {};
    while (readSoFar < HEADER_SIZE) {
      ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
      if (result == -1)
	throw std::runtime_error(utility::createErrorString());
      else if (result == 0) {
	Info << name << "-EOF" << '\n';
	return false;
      }
      else
	readSoFar += result;
    }
    if (!deserialize(header, buffer))
      return false;
    std::size_t payload1Size = extractPayloadSize(header);
    payload1.resize(payload1Size);
    readString(fd, payload1.data(), payload1Size);
    std::size_t payload2Size = extractParameter(header);
    if (payload2Size > 0) {
      payload2.resize(payload2Size);
      readString(fd, payload2.data(), payload2Size);
    }
    return true;
  }

  template <typename T>
  static void readString(int fd, T* received, std::size_t size) {
    std::size_t readSoFar = 0;
    while (readSoFar < size) {
	ssize_t result = read(fd, received + readSoFar, size - readSoFar);
	if (result == -1) {
	  switch (errno) {
	  case EAGAIN:// happens in non blocking mode
	    pollFd(fd, POLLIN);
	    continue;
	    break;
	  default:
	    throw std::runtime_error(utility::createErrorString());
	    break;
	  }
	}
	else
	  readSoFar += result;
      }
  }

  template <typename B>
  static void writeString(int fd, B& body) {
    std::size_t written = 0;
    while (written < body.size()) {
      ssize_t result = write(fd, body.data() + written, body.size() - written);
      if (result == -1) {
	switch (errno) {
	case EAGAIN:// happens in non blocking mode
	  break;
	default:
	  throw std::runtime_error(utility::createErrorString());
	  break;
	}
      }
      else
	written += result;
    }
  }

  template <typename P1, typename P2 = P1>
  static bool sendMsg(std::string_view name,
		      const HEADER& header,
		      const P1& payload1,
		      const P2& payload2 = P2()) {
    int fdWrite = openWriteNonBlock(name);
    utility::CloseFileDescriptor cfdw(fdWrite);
    if (fdWrite == -1)
      return false;
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::string_view headerView(headerBuffer, HEADER_SIZE);
    writeString(fdWrite, headerView);
    writeString(fdWrite, payload1);
    writeString(fdWrite, payload2);
    return true;
  }

  static bool sendMessage(std::string_view name, std::string_view payload);
  static bool readMessage(std::string_view name, std::string& payload, bool nonblock = false);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

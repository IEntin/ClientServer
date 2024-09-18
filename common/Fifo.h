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
    char buffer[HEADER_SIZE] = {};
    readString(fd, buffer, HEADER_SIZE);
    if (!deserialize(header, buffer))
      return false;
    std::size_t payload1Size = extractPayloadSize(header);
    std::size_t payload2Size = extractParameter(header);
    payload1.resize(payload1Size + payload2Size);
    readString(fd, payload1.data(), payload1.size());
    payload2 = { reinterpret_cast<decltype(payload2.data())>(payload1.data()) + payload1Size, payload2Size };
    payload1 = { payload1.data(), payload1Size };
    return true;
  }

  template <typename T>
  static void readString(int fd, T* received, std::size_t size) {
    std::size_t readSoFar = 0;
    while (readSoFar < size) {
	ssize_t result = read(fd, received + readSoFar, size - readSoFar);
	if (result == -1) {
	  switch (errno) {
	  case EAGAIN:
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

  template <typename S>
  static void writeString(int fd, S& str) {
    std::size_t written = 0;
    while (written < str.size()) {
      ssize_t result = write(fd, str.data() + written, str.size() - written);
      if (result == -1) {
	switch (errno) {
	case EAGAIN:
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
  static bool readMessage(std::string_view name, std::string& payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

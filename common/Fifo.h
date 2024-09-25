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
		      bool block,
		      HEADER& header,
		      P1& payload1,
		      P2&& payload2 = P2()) {
    static thread_local std::string payload;
    payload.erase(0);
    if (block) {
      if (!readStringBlock(name, payload))
	  return false;
    }
    else {
      if (!readStringNonBlock(name, payload))
	return false;
    }
    if (!deserialize(header, payload.data()))
      return false;
    printHeader(header, LOG_LEVEL::INFO);
    std::size_t payload2Size = extractParameter(header);
    std::size_t payload1Size = payload.size() - HEADER_SIZE - payload2Size;
    if (payload1Size > 0)
      payload1 =
	{ reinterpret_cast<decltype(payload1.data())>(payload.data()) + HEADER_SIZE, payload1Size };
    if (payload2Size > 0)
      payload2 =
	{ reinterpret_cast<decltype(payload2.data())>(payload.data()) + HEADER_SIZE + payload1Size, payload2Size };
    return true;
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
    writeString(fdWrite, utility::ENDOFMESSAGE);
    return true;
  }

  static bool sendMsgNonBlock(std::string_view name, std::string_view payload);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>
#include <poll.h>

#include "Utility.h"

namespace fifo {

class Fifo {
public:
  template <typename P1, typename P2>
  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      P1& payload1,
		      P2& payload2) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    if (!readMessage(name, block, payload))
      return false;
    if (!deserialize(header, payload.data()))
      return false;
    printHeader(header, LOG_LEVEL::INFO);
    std::size_t payload1Size = extractUncompressedSize(header);;
    std::size_t payload2Size = extractParameter(header);
    payload1.resize(payload1Size);
    payload2.resize(payload2Size);
    unsigned shift = HEADER_SIZE;
    if (payload1Size > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Size);
    shift += payload1Size;
    if (payload2Size > 0)
      std::memcpy(payload2.data(), payload.data() + shift, payload2Size);
    return true;
  }

  template <typename P1, typename P2, typename P3>
  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      P1& payload1,
		      P2& payload2,
		      P3& payload3) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    if (!readMessage(name, block, payload))
      return false;
    if (!deserialize(header, payload.data()))
      return false;
    printHeader(header, LOG_LEVEL::INFO);
    unsigned payload1Size = extractReservedSz(header);
    unsigned payload2Size = extractUncompressedSize(header);
    unsigned payload3Size = extractParameter(header);
    payload1.resize(payload1Size);
    payload2.resize(payload2Size);
    payload3.resize(payload3Size);
    unsigned shift = HEADER_SIZE;
    if (payload1Size > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Size);
    shift += payload1Size;
    if (payload2Size > 0)
      std::memcpy(payload2.data(), payload.data() + shift, payload2Size);
    shift += payload2Size;
    if (payload3Size > 0)
      std::memcpy(payload3.data(), payload.data() + shift, payload3Size);
    return true;
  }

  template <typename P1>
  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      P1& payload1) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    if (!readMessage(name, block, payload))
      return false;
    if (!deserialize(header, payload.data()))
      return false;
    std::size_t payload1Size = payload.size() - HEADER_SIZE;
    if (payload1Size > 0) {
      payload1.resize(payload1Size);
      std::memcpy(payload1.data(), payload.data() + HEADER_SIZE, payload1Size);
      return true;
    }
    return false;
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

  template <typename P1, typename P2 = P1, typename P3 = P2>
  static bool sendMsg(bool block,
		      std::string_view name,
		      const HEADER& header,
		      const P1& payload1,
		      const P2& payload2 = P2(),
		      const P3& payload3 = P3()) {
    int fdWrite = -1;
    if (block)
      fdWrite = open(name.data(), O_WRONLY);
    else
      fdWrite = openWriteNonBlock(name);
    if (fdWrite == -1)
      return false;
    utility::CloseFileDescriptor cfdw(fdWrite);
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    payload.resize(HEADER_SIZE + payload1.size() + payload2.size() + payload3.size());
    std::memcpy(payload.data(), headerBuffer, HEADER_SIZE);
    unsigned shift = HEADER_SIZE;
    std::memcpy(payload.data() + shift, payload1.data(), payload1.size());
    shift += payload1.size();
    if (!payload2.empty())
      std::memcpy(payload.data() + shift, payload2.data(), payload2.size());
    shift += payload2.size();
    if (!payload3.empty())
      std::memcpy(payload.data() + shift, payload3.data(), payload3.size());
    writeString(fdWrite, payload);
    return true;
  }

  static bool readMessage(std::string_view name, bool block, std::string& payload);
  static void onExit(std::string_view fifoName);
private:
  Fifo() = delete;
  ~Fifo() = delete;
  static short pollFd(int fd, short expected);
  static bool setPipeSize(int fd);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
};

} // end of namespace fifo

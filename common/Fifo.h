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
  template <typename P1, typename P2>
  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      P1& payload1,
		      P2&& payload2) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
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
  static bool readMsg1(std::string_view name,
		       bool block,
		       HEADER& header,
		       P1& payload1) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    if (block) {
      if (!readStringBlock(name, payload))
	  return false;
    }
    else {
      if (!readStringNonBlock(name, payload))
	return false;
    }
    if (payload.ends_with(ENDOFMESSAGE))
      payload.erase(payload.cend() - ENDOFMESSAGESZ);
    if (!deserialize(header, payload.data()))
      return false;
    std::size_t payload1Size = payload.size() - HEADER_SIZE;
    payload1.resize(payload1Size);
    unsigned shift = HEADER_SIZE;
    if (payload1Size > 0)
      std::memcpy(payload1.data(), payload.data() + shift, payload1Size);
    return true;
  }

  template <typename P1>
  static bool readMsgUntil(std::string_view name,
			   bool block,
			   HEADER& header,
			   P1& payload1) {
    static thread_local std::string payload;
    payload.erase(payload.cbegin(), payload.cend());
    int fdRead = -1;
    if (block)
      fdRead = open(name.data(), O_RDONLY);
    else
      fdRead = open(name.data(), O_RDONLY | O_NONBLOCK);
    if (fdRead == -1)
      return false;
    utility::CloseFileDescriptor cfdw(fdRead);
    if (!readUntil(fdRead, payload))
      return false;
    if (!deserialize(header, payload.data()))
      return false;
    std::size_t payload1Size = extractUncompressedSize(header);
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

  template <typename P1, typename P2 = P1>
  static bool sendMsg(bool block,
		      std::string_view name,
		      const HEADER& header,
		      const P1& payload1,
		      const P2& payload2 = P2()) {
    int fdWrite = -1;
    if (block)
      fdWrite = open(name.data(), O_WRONLY);
    else
      fdWrite = openWriteNonBlock(name);
    if (fdWrite == -1)
      return false;
    utility::CloseFileDescriptor cfdw(fdWrite);
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::string_view headerView(headerBuffer, HEADER_SIZE);
    writeString(fdWrite, headerView);
    if (!payload1.empty())
      writeString(fdWrite, payload1);
    if (!payload2.empty())
      writeString(fdWrite, payload2);
    return true;
  }

  template <typename P1, typename P2, typename P3>
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
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    std::string_view headerView(headerBuffer, HEADER_SIZE);
    writeString(fdWrite, headerView);
    if (!payload1.empty())
      writeString(fdWrite, payload1);
    if (!payload2.empty())
      writeString(fdWrite, payload2);
    if (!payload3.empty())
      writeString(fdWrite, payload3);
    writeString(fdWrite, ENDOFMESSAGE);
    return true;
  }

  static bool sendMsg(bool block, std::string_view name, std::string_view payload);
  static bool readUntil(int fd, std::string& payload);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

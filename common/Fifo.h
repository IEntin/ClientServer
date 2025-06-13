/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>

#include "Header.h"
#include "IOUtility.h"

namespace fifo {

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

class Fifo {
public:
  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      std::string& payload1,
		      std::vector<unsigned char>& payload2) {
    _payload.clear();
    if (!readMessage(name, block, _payload))
      return false;
    if (!deserialize(header, _payload.data()))
      return false;
    printHeader(header, LOG_LEVEL::INFO);
    std::size_t payload1Size = extractUncompressedSize(header);
    std::size_t payload2Size = extractParameter(header);
    payload1.resize(payload1Size);
    payload2.resize(payload2Size);
    unsigned shift = HEADER_SIZE;
    if (payload1Size > 0)
      std::copy(_payload.begin() + shift, _payload.begin() + shift + payload1Size, payload1.begin());
    shift += payload1Size;
    if (payload2Size > 0)
      std::copy(_payload.begin() + shift, _payload.begin() + shift + payload2Size, payload2.begin());
    return true;
  }

  static bool readMsg(std::string_view name,
		      bool block,
		      HEADER& header,
		      std::string& payload1) {
    _payload.clear();
    if (!readMessage(name, block, _payload))
      return false;
    if (!deserialize(header, _payload.data()))
      return false;
    std::size_t payload1Size = _payload.size() - HEADER_SIZE;
    if (payload1Size > 0) {
      payload1.resize(payload1Size);
      std::copy(_payload.begin() + HEADER_SIZE, _payload.begin() + HEADER_SIZE + payload1Size, payload1.begin());
      return true;
    }
    return false;
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
    CloseFileDescriptor cfdw(fdWrite);
    _payload.clear();
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    _payload.resize(HEADER_SIZE + payload1.size() + payload2.size() + payload3.size());
    std::copy(std::begin(headerBuffer), std::end(headerBuffer), _payload.begin());
    unsigned shift = HEADER_SIZE;
    std::copy(payload1.cbegin(), payload1.cend(), _payload.begin() + shift);
    shift += payload1.size();
    if (!payload2.empty())
      std::copy(payload2.cbegin(), payload2.cend(), _payload.begin() + shift);
    shift += payload2.size();
    if (!payload3.empty())
      std::copy(payload3.cbegin(), payload3.cend(), _payload.begin() + shift);
    writeString(fdWrite, _payload);
    return true;
  }

  static bool readMessage(std::string_view name, bool block, std::string& payload);
  static bool readMessage(std::string_view name,
		      bool block,
		      HEADER& header,
		      std::vector<unsigned char>& payload1,
		      std::vector<unsigned char>& payload2,
		      std::vector<unsigned char>& payload3);
  static void onExit(std::string_view fifoName);
  static void writeString(int fd, std::string_view str);
private:
  Fifo() = delete;
  ~Fifo() = delete;
  static thread_local std::string _payload;
  static short pollFd(int fd, short expected);
  static bool setPipeSize(int fd);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
};

} // end of namespace fifo

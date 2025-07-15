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
  static bool readMessage(std::string_view name,
			  bool block,
			  HEADER& header,
			  std::string& payload1,
			  std::vector<unsigned char>& payload2);

  static bool readMessage(std::string_view name,
			  bool block,
			  HEADER& header,
			  std::string& payload);

  template <typename P1, typename P2 = P1, typename P3 = P1>
  static bool sendMessage(bool block,
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
    std::copy(payload1.begin(), payload1.end(), _payload.begin() + shift);
    shift += payload1.size();
    if (!payload2.empty())
      std::copy(payload2.begin(), payload2.end(), _payload.begin() + shift);
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

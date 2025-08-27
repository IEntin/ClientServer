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
    _payload.insert(_payload.end(), std::cbegin(headerBuffer), std::cend(headerBuffer));
    _payload.insert(_payload.end(), payload1.cbegin(), payload1.cend());
    if (!payload2.empty())
      _payload.insert(_payload.end(), payload2.begin(), payload2.end());
    if (!payload3.empty())
      _payload.insert(_payload.end(), payload3.cbegin(), payload3.cend());
    writeString(fdWrite, _payload);
    return true;
  }

  static bool readMessage(std::string_view name, bool block, std::string& payload);

  static bool readMessage(std::string_view name,
			  bool block,
			  HEADER& header,
			  std::span<std::reference_wrapper<std::string>> array);

  static bool readMessage(std::string_view name,
			  bool block,
			  HEADER& header,
			  std::string& field1,
			  std::string& field2 = _emptyString,
			  std::string& field3 = _emptyString);

  static void onExit(std::string_view fifoName);
  static void writeString(int fd, std::string_view str);
private:
  Fifo() = delete;
  ~Fifo() = delete;
  static thread_local std::string _payload;
  static std::string _emptyString;
  static short pollFd(int fd, short expected);
  static bool setPipeSize(int fd);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
};

} // end of namespace fifo

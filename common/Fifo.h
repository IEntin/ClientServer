/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>

#include "Header.h"
#include "IOUtility.h"
#include "Utility.h"

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
    char headerBuffer[HEADER_SIZE];
    serialize(header, headerBuffer);
    writeString(fdWrite, headerBuffer, std::ssize(headerBuffer));
    writeString(fdWrite, payload1.data(), payload1.size());
    writeString(fdWrite, payload2.data(), payload2.size());
    writeString(fdWrite, payload3.data(), payload3.size());
    writeString(fdWrite, ENDOFMESSAGE.data(), ENDOFMESSAGESZ);
    return true;
  }

  static bool readMessage(std::string_view name, bool block, std::string& payload);

  static bool readMessage(std::string_view name,
			  bool block,
			  HEADER& header,
			  std::span<std::reference_wrapper<std::string>> array);
  static void onExit(std::string_view fifoName);
  static bool writeString(int fd, const char* str, std::size_t size);
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>

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
    std::array<boost::asio::const_buffer, 5> buffers{ boost::asio::buffer(headerBuffer),
						      boost::asio::buffer(payload1),
						      boost::asio::buffer(payload2),
						      boost::asio::buffer(payload3),
						      boost::asio::buffer(ENDOFMESSAGE) };
    writeString(fdWrite, static_cast<const char*>(buffers[0].data()), buffers[0].size());
    writeString(fdWrite, static_cast<const char*>(payload1.data()), payload1.size());
    writeString(fdWrite, static_cast<const char*>(payload2.data()), payload2.size());
    writeString(fdWrite, static_cast<const char*>(payload3.data()), payload3.size());
    writeString(fdWrite, static_cast<const char*>(ENDOFMESSAGE.data()), ENDOFMESSAGESZ);
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
  static short pollFd(int fd, short expected);
  static bool setPipeSize(int fd);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
  static bool readStringBlock(std::string_view name, std::string& payload);
  static bool readStringNonBlock(std::string_view name, std::string& payload);
};

} // end of namespace fifo

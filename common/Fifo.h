/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <cryptopp/secblock.h>

#include "Header.h"
#include "Utility.h"

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int fd, short expected);
  static void readString(int fd, char* received, std::size_t size);
 public:
  static bool readMsgNonBlock(std::string_view name, HEADER& header, std::string& body);
  static bool readMsgNonBlock(std::string_view name, std::string& payload);

  static bool readMsgBlock(std::string_view name, HEADER& header, std::string& body);
  static void readMsgBlock(std::string_view name, std::string& payload);

  template <typename B>
  static void writeString(int fd, B& body) {
    std::size_t written = 0;
    while (written < body.size()) {
      ssize_t result = write(fd, body.data() + written, body.size() - written);
      if (result == -1) {
	if (errno == EAGAIN)
	  continue;
	else
	  throw std::runtime_error(std::strerror(errno));
      }
      else
	written += result;
    }
  }

  template <typename B>
  static bool sendMsg(std::string_view name,
		      const HEADER& header,
		      B& body) {
    int fdWrite = openWriteNonBlock(name);
    utility::CloseFileDescriptor cfdw(fdWrite);
    if (fdWrite == -1)
      return false;
    char buffer[HEADER_SIZE] = {};
    encodeHeader(buffer, header);
    std::string_view view(buffer, HEADER_SIZE);
    writeString(fdWrite, view);
    writeString(fdWrite, body);
    return true;
  }

  static bool sendMsg(std::string_view name, std::string_view payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

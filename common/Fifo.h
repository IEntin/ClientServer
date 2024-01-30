/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <vector>

#include "Header.h"

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int fd, short expected);
  static bool readString(int fd, char* received, std::size_t size);
 public:
  static bool readMsgNonBlock(std::string_view name, HEADER& header, std::vector<char>& body);

  static bool readMsgBlock(std::string_view name, HEADER& header, std::string& body);
  static bool writeString(int fd, std::string_view str);
  static bool sendMsg(std::string_view name, const HEADER& header, std::string_view body = {});
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

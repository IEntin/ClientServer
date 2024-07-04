/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>

#include "Header.h"

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
  static void writeString(int fd, std::string_view str);
  static bool sendMsg(std::string_view name, const HEADER& header, std::string_view body = {});
  static bool sendMsg(std::string_view name, std::string_view payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

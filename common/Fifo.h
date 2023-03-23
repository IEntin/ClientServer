/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string_view>

struct Options;

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;
public:
  // non blocking
  static HEADER readHeader(std::string_view name, int& fd, const Options& options);
  // blocking
  static HEADER readHeader(int fd, const Options& options);

  static bool readString(int fd, char* received, size_t size, const Options& options);
  static bool writeString(int fd, std::string_view str);
  static bool sendMsg(int fd, const HEADER& header, std::string_view body = std::string_view());
  static bool setPipeSize(int fd, long requested);
  static void onExit(std::string_view fifoName, const Options& options);
  static int openWriteNonBlock(std::string_view fifoName, const Options& options);
  static int openReadNonBlock(std::string_view fifoName);
  static short pollFd(int& fd, short expected, const Options& options);
};

} // end of namespace fifo

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string_view>
#include <vector>

struct Options;

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;
public:
  // non blocking
  static HEADER readHeader(std::string_view name, int& fd);
  // non blocking
  static bool readMsg(std::string_view name,
		      int& fd,
		      HEADER& header,
		      std::vector<char>& body);
  // blocking
  static HEADER readHeader(int fd);

  static bool readString(int fd, char* received, size_t size);
  static bool writeString(int fd, std::string_view str);
  static bool sendMsg(int fd, const HEADER& header, std::string_view body = std::string_view());
  static bool setPipeSize(int fd, long requested);
  static void onExit(std::string_view fifoName, const Options& options);
  static int openWriteNonBlock(std::string_view fifoName, const Options& options, int repeat = 0);
  static int openReadNonBlock(std::string_view fifoName);
  static short pollFd(int& fd, short expected);
};

} // end of namespace fifo

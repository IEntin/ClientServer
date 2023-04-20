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

  static short pollFd(int fd, short expected);
  static bool readString(int fd, char* received, size_t size);
 public:
  static bool readMsgNonBlock(std::string_view name,
			      HEADER& header,
			      std::vector<char>& body);

  static bool readMsgBlock(std::string_view name,
			   HEADER& header,
			   std::vector<char>& body);
  static bool writeString(int fd, std::string_view str);
  static bool sendMsg(std::string_view name,
		      const HEADER& header,
		      const Options& options,
		      std::string_view body = std::string_view());
  static bool setPipeSize(int fd, long requested);
  static void onExit(std::string_view fifoName, const Options& options);
  static int openWriteNonBlock(std::string_view fifoName, const Options& options);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

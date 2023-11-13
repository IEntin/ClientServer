/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <string_view>
#include <any>
#include <vector>

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int fd, short expected);
  static bool readString(int fd, char* received, size_t size);
 public:
  static bool readMsgNonBlock(std::string_view name,
			      const std::any& options,
			      HEADER& header,
			      std::vector<char>& body);

  static bool readMsgBlock(std::string_view name,
			   const std::any& options,
			   HEADER& header,
			   std::string& body);
  static bool writeString(int fd, std::string_view str);
  static bool sendMsg(std::string_view name,
		      const std::any& options,
		      const HEADER& header,
		      std::string_view body = {});
  static bool setPipeSize(int fd, const std::any& options);
  static void onExit(std::string_view fifoName, const std::any& options);
  static int openWriteNonBlock(std::string_view fifoName, const std::any& options);
  static int openReadNonBlock(std::string_view fifoName, const std::any& options);
};

} // end of namespace fifo

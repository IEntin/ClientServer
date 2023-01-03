/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"

struct Options;

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int& fd, short expected, const Options& options);
public:
  static HEADER readHeader(int fd, const Options& options);

  static bool readString(int fd, char* received, size_t size, const Options& options);

  static bool writeString(int fd, std::string_view str);

  static bool setPipeSize(int fd, long requested);

  static void onExit(std::string_view fifoName, const Options& options);
};

} // end of namespace fifo

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int& fd, short expected, int maxRepeatEINTR);
public:
  static HEADER readHeader(int fd, int numberRepeatEINTR);

  static bool readString(int fd, char* received, size_t size, int maxRepeatEINTR);

  static bool writeString(int fd, std::string_view str);

  static bool setPipeSize(int fd, long requested);

  static void onExit(std::string_view fifoName, int numberRepeatENXIO, int ENXIOwait);

  static bool exists(std::string_view name);
};

} // end of namespace fifo

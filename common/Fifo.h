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

  static void onExit(const std::string& fifoName, int numberRepeatENXIO, int ENXIOwait);

};

} // end of namespace fifo

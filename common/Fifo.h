/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

enum class COMPRESSORS : short;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

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

};

} // end of namespace fifo

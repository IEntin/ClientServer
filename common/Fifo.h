/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

enum class COMPRESSORS : short unsigned int;

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

using Batch = std::vector<std::string>;

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int& fd, short expected, std::string_view fifoName, int maxRepeatEINTR);
public:
  static HEADER readHeader(int fd, std::string_view fifoName, int numberRepeatEINTR);

  static bool readString(int fd, char* received, size_t size, std::string_view fifoName, int maxRepeatEINTR);

  static bool writeString(int fd, std::string_view str);

  static bool setPipeSize(int fd, long requested);

};

} // end of namespace fifo

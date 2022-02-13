/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include <vector>

using Batch = std::vector<std::string>;

namespace fifo {

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static ssize_t getDefaultPipeSize();

  static const ssize_t _defaultPipeSize;
public:
  static HEADER readHeader(int fd);

  static bool readString(int fd, char* received, size_t size);

  static bool writeString(int fd, std::string_view str);

};

} // end of namespace fifo

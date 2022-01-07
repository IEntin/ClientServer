/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Utility.h"

namespace fifo {

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static bool readBatch(int fd,
			size_t uncomprSize,
			size_t comprSize,
			bool bcompressed,
			Batch& batch);

  static bool readVectorChar(int fd,
			     size_t uncomprSize,
			     size_t comprSize,
			     bool bcompressed,
			     std::vector<char>& uncompressed);

  static ssize_t getDefaultPipeSize();
  static const ssize_t _defaultPipeSize;
public:
  static HEADER readHeader(int fd);
  static bool readString(int fd, char* received, size_t size);
  static bool writeString(int fd, std::string_view str);
  static bool sendReply(int fd, Batch& batch);
  static bool receive(int fd, Batch& batch);
  static bool receive(int fd, std::vector<char>& uncompressed);
};

} // end of namespace fifo

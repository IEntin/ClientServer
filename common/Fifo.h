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

  static bool readBatch(int fd,
			size_t uncomprSize,
			size_t comprSize,
			bool bcompressed,
			std::ostream* dataStream);

  static bool writeString(int fd, std::string_view str);

  static bool sendReply(int fd, Batch& batch);

  static bool receive(int fd, std::vector<char>& uncompressed, HEADER& header);
};

} // end of namespace fifo

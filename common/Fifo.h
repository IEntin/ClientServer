/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Header.h"
#include "Utility.h"
#include <fcntl.h>
#include <string_view>
#include <unistd.h>

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

  template <typename B>
  static bool readMsgBlock(std::string_view name,
			   HEADER& header,
			   B& body) {
    int fd = open(name.data(), O_RDONLY);
    utility::CloseFileDescriptor cfdr(fd);
    if (fd == -1) {
      LogError << name << '-' << std::strerror(errno) << '\n';
      return false;
    }
    size_t readSoFar = 0;
    char buffer[HEADER_SIZE] = {};
    while (readSoFar < HEADER_SIZE) {
      ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
      if (result == -1) {
	if (errno == EAGAIN || errno == EWOULDBLOCK) {
	  continue;
	}
	else {
	  LogError << std::strerror(errno) << '\n';
	  throw std::runtime_error(std::strerror(errno));
	}
      }
      else if (result == 0) {
	Debug << (errno ? std::strerror(errno) : "EOF") << '\n';
	return false;
      }
      else
	readSoFar += static_cast<size_t>(result);
    }
    if (readSoFar != HEADER_SIZE) {
      LogError << "HEADER_SIZE=" << HEADER_SIZE
	       << " readSoFar=" << readSoFar << '\n';
      throw std::runtime_error(std::strerror(errno));
    }
    header = decodeHeader(buffer);
    if (!isOk(header)) {
      LogError << "header is invalid." << '\n';
      return false;
    }
    size_t payloadSize = extractPayloadSize(header);
    body.resize(payloadSize);
    return readString(fd, body.data(), payloadSize);
  }

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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <fcntl.h>
#include <cryptopp/secblock.h>

#include "Header.h"
#include "Logger.h"
#include "Utility.h"

namespace fifo {

class Fifo {
  Fifo() = delete;
  ~Fifo() = delete;

  static short pollFd(int fd, short expected);
public:
  static bool readMsgNonBlock(std::string_view name, HEADER& header, std::string& body);
  static bool readMsgNonBlock(std::string_view name, std::string& payload);

  static void readMsgBlock(std::string_view name, std::string& payload);

  template <typename P1, typename P2>
  static bool readMsgBlock(std::string_view name,
			   HEADER& header,
			   P1& payload1,
			   P2& payload2) {
    int fd = open(name.data(), O_RDONLY);
    utility::CloseFileDescriptor cfdr(fd);
    std::size_t readSoFar = 0;
    char buffer[HEADER_SIZE] = {};
    while (readSoFar < HEADER_SIZE) {
      ssize_t result = read(fd, buffer + readSoFar, HEADER_SIZE - readSoFar);
      if (result == -1) {
	if (errno != EAGAIN)
	  throw std::runtime_error(std::strerror(errno));
      }
      else if (result == 0) {
	Info << name << "-EOF" << '\n';
	return false;
      }
      else
	readSoFar += result;
    }
    header = decodeHeader(buffer);
    std::size_t payload1Size = extractPayloadSize(header);
    payload1.resize(payload1Size);
    readString(fd, payload1.data(), payload1Size);
    std::size_t payload2Size = extractParameter(header);
    if (payload2Size > 0) {
      payload2.resize(payload2Size);
      readString(fd, payload2.data(), payload2Size);
    }
    return true;
  }

  template <typename T>
  static void readString(int fd, T* received, std::size_t size) {
    std::size_t readSoFar = 0;
    while (readSoFar < size) {
      ssize_t result = read(fd, received + readSoFar, size - readSoFar);
      if (result == -1) {
	if (errno != EAGAIN)
	  throw std::runtime_error(std::strerror(errno));
      }
      else
	readSoFar += result;
    }
  }

  template <typename B>
  static void writeString(int fd, B& body) {
    std::size_t written = 0;
    while (written < body.size()) {
      ssize_t result = write(fd, body.data() + written, body.size() - written);
      if (result == -1) {
	if (errno == EAGAIN)
	  continue;
	else
	  throw std::runtime_error(std::strerror(errno));
      }
      else
	written += result;
    }
  }

  template <typename P1, typename P2>
  static bool sendMsg(std::string_view name,
		      const HEADER& header,
		      const P1& payload1,
		      const P2& payload2) {
    int fdWrite = openWriteNonBlock(name);
    utility::CloseFileDescriptor cfdw(fdWrite);
    if (fdWrite == -1)
      return false;
    char headerBuffer[HEADER_SIZE] = {};
    encodeHeader(headerBuffer, header);
    std::string_view headerView(headerBuffer, HEADER_SIZE);
    writeString(fdWrite, headerView);
    writeString(fdWrite, payload1);
    writeString(fdWrite, payload2);
    return true;
  }
  static bool sendMsg(std::string_view name, std::string_view payload);
  static bool setPipeSize(int fd);
  static void onExit(std::string_view fifoName);
  static int openWriteNonBlock(std::string_view fifoName);
  static int openReadNonBlock(std::string_view fifoName);
};

} // end of namespace fifo

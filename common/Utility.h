/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

// common constants

constexpr std::size_t MAXBUFFERSIZE = 10000000;
constexpr std::string_view buildDateTime = __DATE__ " " __TIME__;
constexpr std::string_view ENDOFMESSAGE("e10c82c380024fbe8e2b1f578c8793db");
constexpr std::size_t ENDOFMESSAGESZ = ENDOFMESSAGE.size();
constexpr const char* FIFO_NAMED_MUTEX("FIFO_NAMED_MUTEX");

namespace utility {

// INPUT can be a string or string_view.
// CONTAINER can be a vector or a deque or a list of string,
// string_view, vector<char> or vector of objects of any
// class with constructor over the range [first, last)

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
  std::size_t start = 0;
  while (start < input.size()) {
    std::size_t next = input.find(delim, start);
    bool endOfInput = next == INPUT::npos;
    rows.emplace_back(input.cbegin() + start,
      endOfInput ? input.cend() : input.cbegin() + next + keepDelim);
    if (endOfInput)
      break;
    start = next + 1;
  }
}

// reversed container order to erase from the end of the input
template <typename CONTAINER>
[[maybe_unused]] static void splitReversedOrder(std::string_view input,
						CONTAINER& rows,
						char delim = '\n',
						int keepDelim = 0) {
  std::size_t start = 0;
  while (start < input.size()) {
    std::size_t next = input.find(delim, start);
    bool endOfInput = next == std::string_view::npos;
    rows.emplace_front(input.cbegin() + start,
      endOfInput ? input.cend() : input.cbegin() + next + keepDelim);
    if (endOfInput)
      break;
    start = next + 1;
  }
}

template <typename INPUT, typename CONTAINER>
std::size_t splitReuseVector(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
  unsigned index = 0;
  std::size_t start = 0;
  while (start < input.size()) {
    std::size_t next = input.find(delim, start);
    bool endOfInput = next == INPUT::npos;
    if (index >= rows.size())
      rows.emplace_back();
    rows[index] = { input.cbegin() + start,
		    endOfInput ? input.cend() : input.cbegin() + next + keepDelim };
    if (endOfInput)
      break;
    ++index;
    start = next + 1;
  }
  return index;
}

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, const char* separators) {
  std::size_t beg = input.find_first_not_of(separators);
  while (beg != INPUT::npos) {
    std::size_t pos = input.find_first_of(separators, beg + 1);
    std::size_t end = pos == INPUT::npos ? input.size() : pos;
    rows.emplace_back(input.cbegin() + beg, input.cbegin() + end);
    beg = input.find_first_not_of(separators, end + 1);
  }
}

template <typename BUFFER>
void readFile(std::string_view fileName, BUFFER& buffer) {
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  std::uintmax_t size = std::filesystem::file_size(fileName);
  buffer.resize(size);
  stream.read(&buffer[0], size);
}

template <typename SOURCE>
bool writeToFd(int fd, SOURCE& source) {
  try {
    boost::asio::io_context io_context;
    boost::asio::posix::stream_descriptor sd(io_context, fd);
    std::vector<boost::asio::const_buffer> buffers;
    buffers.push_back(boost::asio::buffer(source));
    boost::asio::write(sd, buffers);
    return true;
  }
  catch (const boost::system::system_error& e) {
    LogError<< e.what() << '\n';
    return false;
  }
}

std::size_t getUniqueId();

std::string generateRawUUID();

void setServerTerminal(std::string_view terminal);
void setClientTerminal(std::string_view terminal);
void setTestbinTerminal(std::string_view terminal);

bool isServerTerminal();
bool isClientTerminal();
bool isTestbinTerminal();
void removeAccess();

} // end of namespace utility

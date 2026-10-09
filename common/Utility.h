/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <ranges>
#include <string_view>

#include <boost/asio.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>

// common constants

constexpr std::string_view ENDOFMESSAGE("f65438b3bf504ace8483e6642a84d2fd");
constexpr std::size_t ENDOFMESSAGESZ = ENDOFMESSAGE.size();
constexpr const char* FIFO_NAMED_MUTEX("FIFO_NAMED_MUTEX");

namespace utility {

consteval std::string_view getAuthenticationMessage() {
  static constexpr std::string_view message = __DATE__ " " __TIME__;
  return message;
}

// INPUT can be a string or string_view.
// CONTAINER can be a vector or a deque or a list of string,
// string_view, vector<char> or vector of objects of any
// class with constructor over the range [first, last)

template <typename INPUT, typename CONTAINER>
std::size_t splitRanges(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
  rows = input
    | std::views::split(delim)
    | std::views::transform([&](auto&& rng) {
      const char* start = &*rng.begin();
      size_t size = std::ranges::distance(rng);
      if (keepDelim == 1) {
	if (start + size + 1 <= input.data() + input.size()) {
	  size += 1;
	}
      }
      return std::string_view(start, size);
    })
    | std::ranges::to<std::vector<std::string_view>>();

  return rows.size();
}

// reversed container order to erase from the end of the input
template <typename INPUT,typename CONTAINER>
void splitReversedOrder(const INPUT& input,
			CONTAINER& rows,
			char delim = '\n',
			int keepDelim = 0) {
  static std::vector<std::string_view> vect;
  vect.clear();
  [[maybe_unused]] std::size_t result = splitRanges(input, vect, delim, keepDelim);
  rows.assign(vect.rbegin(), vect.rend());
}

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, std::string_view separators) {
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
  stream.read(buffer.data(), size);
}

template <typename SOURCE>
bool writeToFd(int fd, SOURCE& source) {
  try {
    boost::asio::io_context io_context;
    boost::asio::posix::stream_descriptor sd(io_context, fd);
    boost::asio::write(sd, boost::asio::buffer(source));
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

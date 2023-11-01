/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Logger.h"
#include <charconv>
#include <cstring>

enum class STATUS : char;

constexpr int CONV_BUFFER_SIZE = 10;

namespace utility {

// INPUT can be a string or string_view.
// CONTAINER can be a vector or a deque or a list of string,
// string_view, vector<char> or vector of objects of any
// class with constructor over the range [first, last)

template <typename INPUT, typename CONTAINER>
  void split(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
    size_t start = 0;
    size_t next = 0;
    while (start < input.size()) {
      next = input.find(delim, start);
      if (next == std::string::npos) {
	if (input.size() > start)
	  rows.emplace_back(input.cbegin() + start, input.cend());
	break;
      }
      else if (next > start)
	rows.emplace_back(input.cbegin() + start, input.cbegin() + next + keepDelim);
      start = next + 1;
    }
}

template <typename INPUT, typename CONTAINER>
  void split(const INPUT& input, CONTAINER& rows, const char* separators) {
    size_t start = input.find_first_not_of(separators);
    size_t end = 0;
    while (start != INPUT::npos) {
      size_t pos = input.find_first_of(separators, start + 1);
      end = pos == INPUT::npos ? input.size() : pos;
      rows.emplace_back(input.data() + start, input.data() + end);
      start = input.find_first_not_of(separators, end + 1);
    }
}

constexpr auto fromChars = []<typename T>(std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc()) {
    LogError << "problem converting str:" << str << '\n';
    throw std::runtime_error("problem converting str");
  }
  return true;
};

template <typename N>
  concept Integral = std::is_integral_v<N>;

template <typename N>
  concept FloatingPoint = std::is_floating_point_v<N>;

template <Integral T>
  void toChars(T value, char* buffer, size_t size = CONV_BUFFER_SIZE) {
    if (auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
	ec != std::errc()) {
      LogError << "problem converting to string:" << value << '\n';
      throw std::runtime_error("problem converting to string");
    }
}

template <Integral N>
void toChars(N value, std::string& target, size_t size = CONV_BUFFER_SIZE) {
  size_t origSize = target.size();
  target.resize(origSize + size);
  size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(target.data() + origSize, target.data() + origSize + size, value);
  sizeIncr = ptr - target.data() - origSize;
  if (ec == std::errc())
    target.resize(origSize + sizeIncr);
  else
    LogError << "problem translating number:" << value << '\n';
}

template <FloatingPoint N>
void toChars(N value, std::string& target, int precision, size_t size = CONV_BUFFER_SIZE) {
  size_t origSize = target.size();
  target.resize(origSize + size);
  size_t sizeIncr = 0;
  auto [ptr, ec] = std::to_chars(target.data() + origSize, target.data() + origSize + size, value,
				 std::chars_format::fixed, precision);
  sizeIncr = ptr - target.data() - origSize;
  if (ec == std::errc())
    target.resize(origSize + sizeIncr);
  else
    LogError << "problem translating number:" << value << '\n';
}

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

std::string getUniqueId();

void readFile(std::string_view name, std::string& buffer);

void writeFile(std::string_view fileName, std::string_view data);

bool getLastLine(std::string_view fileName, std::string& lastLine);

bool fileEndsWithEOL(std::string_view fileName);

} // end of namespace utility

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include "Logger.h"
#include <charconv>
#include <cstring>

enum class STATUS : char;

inline constexpr int CONV_BUFFER_SIZE = 10;

namespace utility {

// INPUT can be a string or string_view.
// CONTAINER can be a vector | deque | list of
// string, string_view, vector<char> or objects of any class
// with constructor arguments (const char*, const char*)

template <typename INPUT, typename CONTAINER>
  void split(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
    size_t start = 0;
    size_t next = 0;
    while (start < input.size()) {
      next = input.find(delim, start);
      if (next == std::string::npos) {
	if (input.size() > start)
	  rows.emplace_back(input.data() + start, input.data() + input.size());
	break;
      }
      else if (next > start)
	rows.emplace_back(input.data() + start, input.data() + next + keepDelim);
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

inline constexpr auto fromChars = []<typename T>(std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc()) {
    LogError << "problem converting str:" << str << '\n';
    throw std::runtime_error("problem converting str");
  }
  return true;
};

template <typename N>
  struct Print {
    explicit Print(N number, int precision = 0) :
      _number(number), _precision(precision) {}
    N _number;
    int _precision;
};

template <typename N>
  concept Integral = std::is_integral_v<N>;

template <typename N>
  concept FloatingPoint = std::is_floating_point_v<N>;

template <Integral N>
  std::ostream& operator<<(std::ostream& os, const Print<N>& value) {
    char arr[CONV_BUFFER_SIZE] = {};
    if (auto [ptr, ec] = std::to_chars(arr, arr + CONV_BUFFER_SIZE, value._number);
	ec == std::errc())
      os.write(arr, ptr - arr);
    else
      LogError << "problem translating number:" << value._number << '\n';
    return os;
}

template <FloatingPoint N>
  std::ostream& operator<<(std::ostream& os, const Print<N>& value) {
    char arr[CONV_BUFFER_SIZE] = {};
    if (auto [ptr, ec] = std::to_chars(arr, arr + CONV_BUFFER_SIZE, value._number,
				       std::chars_format::fixed, value._precision);
 	ec == std::errc())
      os.write(arr, ptr - arr);
    else
      LogError << "problem translating number:" << value._number << '\n';
    return os;
}

template <Integral T>
  void toChars(T value, char* buffer, size_t size) {
    if (auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
	ec != std::errc()) {
      LogError << "problem converting to string:" << value << '\n';
      throw std::runtime_error("problem converting to string");
    }
}

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

std::string getUniqueId();

std::string readFile(const std::string& name);

bool writeFile(const std::string& name, const std::string& contents);

} // end of namespace utility

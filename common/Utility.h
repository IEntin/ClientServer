/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <charconv>
#include <iostream>
#include <string_view>
#include <vector>

inline constexpr unsigned CONV_BUFFER_SIZE = 10;

namespace utility {

// INPUT can be a string or string_view
// OUTPUT a string or string_view

template <typename INPUT, typename OUTPUT>
  void split(const INPUT& input, std::vector<OUTPUT>& lines, char delim = '\n') {
    size_t start = 0;
    size_t next = 0;
    while (start < input.size()) {
      next = input.find(delim, start);
      if (next == std::string::npos) {
	if (input.size() > start + 1)
	  lines.emplace_back(input.data() + start, input.size() - start);
	break;
      }
      else if (next > start + 1)
	lines.emplace_back(input.data() + start, next - start);
      start = next + 1;
    }
}

template <typename VALUE, typename INPUT, typename ROW>
  void split(VALUE ignore, const INPUT& input, std::vector<ROW>& rows, char delim = '\n') {
    size_t start = 0;
    size_t next = 0;
    while (start < input.size()) {
      next = input.find(delim, start);
      if (next == std::string::npos) {
	if (input.size() > start + 1) {
	  VALUE value(input.data() + start, input.size() - start);
	  rows.emplace_back(value);
	}
	break;
      }
      else if (next > start + 1) {
	VALUE value(input.data() + start, next - start);
	rows.emplace_back(value);
      }
      start = next + 1;
    }
}

template <typename STRING1, typename STRING2>
  void split(const STRING1& input, std::vector<STRING2>& lines, const char* separators) {
    size_t start = input.find_first_not_of(separators);
    size_t end = 0;
    while (start != STRING1::npos) {
      size_t pos = input.find_first_of(separators, start + 1);
      end = pos == STRING1::npos ? input.size() : pos;
      lines.emplace_back(input.data() + start, end - start);
      start = input.find_first_not_of(separators, end + 1);
    }
}

inline constexpr auto fromChars = []<typename T>(std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
    << " problem converting str:" << str << std::endl;
    return false;
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
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << value._number << std::endl;
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
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-error translating number:" << value._number << std::endl;
    return os;
}

template <Integral T>
  bool toChars(T value, char* buffer, size_t size) {
    if (auto [ptr, ec] = std::to_chars(buffer, buffer + size, value);
	ec != std::errc()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< "-problem converting to string:" << value << std::endl;
      return false;
    }
    return true;
}

std::string readFile(const std::string& name);

} // end of namespace utility

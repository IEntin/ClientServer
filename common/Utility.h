/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <charconv>
#include <iostream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

inline constexpr unsigned CONV_BUFFER_SIZE = 10;
inline constexpr size_t NUM_FIELD_SIZE = 10;
inline constexpr size_t COMPRESSOR_NAME_SIZE = 4;
inline constexpr size_t DIAGNOSTICS_SIZE = 2;
inline constexpr size_t UNUSED_SIZE = 4;
inline constexpr size_t HEADER_SIZE = NUM_FIELD_SIZE * 2 + COMPRESSOR_NAME_SIZE + DIAGNOSTICS_SIZE + UNUSED_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum HEADER_INDEX {
  UNCOMPRESSED_SIZE,
  COMPRESSED_SIZE,
  COMPRESSOR,
  DIAGNOSTICS,
  DONE
};
using HEADER = std::tuple<ssize_t, ssize_t, std::string_view, bool, bool>;

using Batch = std::vector<std::string>;

namespace utility {

// INPUT can be a string or string_view or vector<char>
// OUTPUT a string or string_view or vector<char>
template <typename INPUT, typename OUTPUT>
  void split(const INPUT& input, std::vector<OUTPUT>& lines, char delim = '\n') {
    auto start = input.cbegin();
    auto end = input.cend();
    auto next = std::find(start, end, delim);
    while (next != end) {
      if (next > start + 1)
	lines.emplace_back(start, next);
      start = next + 1;
      next = std::find(start, end, delim);
    }
    if (next > start + 1)
      lines.emplace_back(start, next);
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

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, std::string_view compressor, bool diagnostics);

HEADER decodeHeader(std::string_view buffer, bool done = true);

std::string createRequestId(size_t index);

bool preparePackage(const Batch& payload, Batch& modified);

bool mergePayload(const Batch& batch, Batch& aggregatedBatch);

bool buildMessage(const Batch& payload, Batch& message, bool diagnostics);

} // end of namespace utility

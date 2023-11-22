/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <charconv>
#include <cstring>
#include <tuple>

#include "CommonConstants.h"
#include "Logger.h"

namespace utility {

// INPUT can be a string or string_view.
// CONTAINER can be a vector or a deque or a list of string,
// string_view, vector<char> or vector of objects of any
// class with constructor over the range [first, last)

// profiler:
// 4.37%     19.28     1.16 14025138     0.00     0.00  void utility::split<std::basic_string_view<char, std::char_traits<char> >, std::vector<std::basic_string_view<char, std::char_traits<char> >, std::allocator<std::basic_string_view<char, std::char_traits<char> > > > >(std::basic_string_view<char, std::char_traits<char> > const&, std::vector<std::basic_string_view<char, std::char_traits<char> >, std::allocator<std::basic_string_view<char, std::char_traits<char> > > >&, char, int)

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
  auto beg = input.cbegin();
  while (beg != input.cend()) {
    auto end = std::find(std::next(beg, 1), input.cend(), delim);
    bool endOfInput = end == input.cend();
    rows.emplace_back(beg, endOfInput ? input.cend() : std::next(end, keepDelim));
    if (endOfInput)
      break;
    beg = std::next(end, 1);
  }
}

// less attractive but 3+ times faster version
// profiler:
// 1.33%     22.75     0.33 13928674     0.00     0.00  void utility::splitFast<std::basic_string_view<char, std::char_traits<char> >, std::vector<std::basic_string_view<char, std::char_traits<char> >, std::allocator<std::basic_string_view<char, std::char_traits<char> > > > >(std::basic_string_view<char, std::char_traits<char> > const&, std::vector<std::basic_string_view<char, std::char_traits<char> >, std::allocator<std::basic_string_view<char, std::char_traits<char> > > >&, char, int)

template <typename INPUT, typename CONTAINER>
void splitFast(const INPUT& input, CONTAINER& rows, char delim = '\n', int keepDelim = 0) {
  size_t start = 0;
  while (start < input.size()) {
    size_t next = input.find(delim, start);
    bool endOfInput = next == INPUT::npos;
    rows.emplace_back(input.cbegin() + start,
      endOfInput ? input.cend() : input.cbegin() + next + keepDelim);
    if (endOfInput)
      break;
    start = next + 1;
  }
}

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, const char* separators) {
  size_t beg = input.find_first_not_of(separators);
  while (beg != INPUT::npos) {
    size_t pos = input.find_first_of(separators, beg + 1);
    size_t end = pos == INPUT::npos ? input.size() : pos;
    rows.emplace_back(input.data() + beg, input.data() + end);
    beg = input.find_first_not_of(separators, end + 1);
  }
}

constexpr auto fromChars = []<typename T>(std::string_view str, T& value) {
  if (auto [p, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
      ec != std::errc()) {
    LogError << "problem converting str:" << str << '\n';
    throw std::runtime_error("problem converting str");
  }
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

using SIZETUPLE = std::tuple<unsigned, unsigned>;

void printSizeKey(const SIZETUPLE& sizeKey, std::string& target);

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

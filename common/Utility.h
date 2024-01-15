/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <cstring>
#include <tuple>

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
    auto end = std::find(std::next(beg), input.cend(), delim);
    bool endOfInput = end == input.cend();
    rows.emplace_back(beg, endOfInput ? input.cend() : std::next(end, keepDelim));
    if (endOfInput)
      break;
    beg = std::next(end);
  }
}

// this version is 3+ times faster.
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

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include <boost/assert/source_location.hpp>

#include "Crypto.h"
#include "Header.h"

// common constants
constexpr std::string_view ENDOFMESSAGE("b7d0d9fc71b943288c99f178ebff6e9d");
static constexpr std::size_t ENDOFMESSAGESZ = ENDOFMESSAGE.size();
constexpr const char* FIFO_NAMED_MUTEX("FIFO_NAMED_MUTEX");

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

template <typename INPUT, typename CONTAINER>
void split(const INPUT& input, CONTAINER& rows, const char* separators) {
  std::size_t beg = input.find_first_not_of(separators);
  while (beg != INPUT::npos) {
    std::size_t pos = input.find_first_of(separators, beg + 1);
    std::size_t end = pos == INPUT::npos ? input.size() : pos;
    rows.emplace_back(input.data() + beg, input.data() + end);
    beg = input.find_first_not_of(separators, end + 1);
  }
}

struct CloseFileDescriptor {
  CloseFileDescriptor(int& fd);
  ~CloseFileDescriptor();
  int& _fd;
};

std::size_t getUniqueId();

int generateRandomNumber(int min = 1, int max = 10000);

std::u8string generateRawUUID();

void readFile(std::string_view name, std::string& buffer);

bool getLastLine(std::string_view fileName, std::string& lastLine);

bool fileEndsWithEOL(std::string_view fileName);

std::string createErrorString(std::errc ec,
			      const boost::source_location& location = BOOST_CURRENT_LOCATION);

std::string createErrorString(const boost::source_location& location = BOOST_CURRENT_LOCATION);

void compressEncrypt(std::string& buffer,
		     bool encrypt,
		     const HEADER& header,
		     CryptoWeakPtr crypto,
		     std::string& data);

void decryptDecompress(std::string& buffer,
		       HEADER& header,
		       CryptoWeakPtr crypto,
		       std::string& data);
void setServerTerminal(std::string_view terminal);
void setClientTerminal(std::string_view terminal);
void setTestbinTerminal(std::string_view terminal);

bool isServerTerminal();
bool isClientTerminal();
bool isTestbinTerminal();
void removeAccess();

} // end of namespace utility

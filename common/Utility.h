/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>

#include "Header.h"

// common constants
constexpr std::string_view ENDOFMESSAGE("94db2297686d43a2b470f7a62d78efa7");
constexpr std::size_t ENDOFMESSAGESZ = ENDOFMESSAGE.size();
constexpr const char* FIFO_NAMED_MUTEX("FIFO_NAMED_MUTEX");

using CryptoWeakPtr = std::weak_ptr<class Crypto>;

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
    rows.emplace_back(input.data() + beg, input.data() + end);
    beg = input.find_first_not_of(separators, end + 1);
  }
}

bool isEncrypted(std::string_view data);

std::size_t getUniqueId();

std::string generateRawUUID();

void readFile(std::string_view name, std::string& buffer);

bool getLastLine(std::string_view fileName, std::string& lastLine);

bool fileEndsWithEOL(std::string_view fileName);

std::string_view compressEncrypt(std::string& buffer,
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

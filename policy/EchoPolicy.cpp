/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoPolicy.h"

#include <boost/regex.hpp>

// tests transport layer, multithreading, compression, and encryption
std::string_view EchoPolicy::operator() (std::string_view request,
					 std::string& buffer) noexcept {
  // regex does not work with string_view
  buffer = request;
  // remove id part, if there is, to run 'diff' with the source file
  boost::regex pattern("^\\[\\d+\\]");
  boost::smatch match;
  if (boost::regex_search(buffer, match, pattern))
    buffer.erase(0, match.str().size());
  buffer.push_back('\n');
  return buffer;
}

std::string_view EchoPolicy::processRequest(const SIZETUPLE& sizeKeyey[[maybe_unused]],
					    std::string_view input,
					    bool diagnostics[[maybe_unused]],
					    std::string& buffer) {
  return operator() (input, buffer);
}

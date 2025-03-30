/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "EchoPolicy.h"

#include <boost/regex.hpp>

thread_local std::string EchoPolicy::_buffer;

// tests transport layer, multithreading, compression, and encryption
std::string_view EchoPolicy::operator() (const Request& request,
					 bool diagnostics[[maybe_unused]]) {
  // regex does not work with string_view
  _buffer = request._value;
  _buffer.push_back('\n');
  // remove id part, if there is, to run 'diff' with the source file
  boost::regex pattern("^\\[\\d+\\]");
  boost::smatch match;
  // string::erase is expensive
  std::string_view result = _buffer;
  if (boost::regex_search(_buffer, match, pattern))
    result.remove_prefix(match.str().size());
  return result;
}

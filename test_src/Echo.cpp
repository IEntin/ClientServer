/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"

// test transport layer, multithreading, compression, and encoding
std::string_view Echo::processRequest(std::string_view request) noexcept {
  std::string_view response(request);
  // remove id part, if there is, to run 'diff' with the source file
  size_t pos = response.find(']');
  if (pos != std::string::npos && response[0] == '[')
    response.remove_prefix(pos + 1);
  static thread_local std::string result;
  result = response;
  result.push_back('\n');
  return result;
}

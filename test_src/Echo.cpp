/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"

// test transport layer, multithreading, and compression
std::string_view Echo::processRequest(std::string_view,
				      std::string_view request,
				      bool) noexcept {
  std::string_view response(request);
  // remove id part, if there is, to run 'diff' with the source file
  size_t pos = response.find(']');
  if (pos != std::string::npos && response[0] == '[')
    response.remove_prefix(pos + 1);
  static thread_local std::string result;
  result.resize(0);
  result.insert(result.end(), response.cbegin(), response.cend());
  result.push_back('\n');
  return { result.data(), result.size() };
}

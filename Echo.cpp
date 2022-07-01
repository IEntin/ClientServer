/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"

namespace echo {

// test transport layer, multithreading, and compression
std::string Echo::processRequest(std::string_view, std::string_view request) noexcept {
  std::string_view response(request);
  // remove id part, if there is, to run 'diff' with the source file
  size_t pos = response.find(']');
  if (pos != std::string::npos && response[0] == '[')
    response.remove_prefix(pos + 1);
  return std::string(response).append(1, '\n');
}

} // end of namespace echo

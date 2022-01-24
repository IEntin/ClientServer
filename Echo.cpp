/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Echo.h"

namespace echo {

// test transport layer, multithreading, and compression

  std::string processRequest(std::string_view view, bool diagnostics) noexcept {
  std::string_view response(view);
  // remove id part, if there is, to run 'diff' with source file
  size_t pos = response.find(']');
  if (pos != std::string::npos && response[0] == '[')
    response.remove_prefix(pos + 1);
  return std::string(response).append(1, '\n');
}

} // end of namespace echo

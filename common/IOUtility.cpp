/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "IOUtility.h"

#include <system_error>

namespace ioutility {

std::string& operator << (std::string& buffer, char c) {
  return buffer += c;
}

std::string& operator << (std::string& buffer, std::string_view str) {
  return buffer += str;
}

std::string& operator << (std::string& buffer, const SIZETUPLE& sizeKey) {
  unsigned width = std::get<0>(sizeKey);
  toChars(width, buffer);
  buffer += 'x';
  unsigned height = std::get<1>(sizeKey);
  toChars(height, buffer);
  return buffer;
}

std::string createErrorString(std::errc ec,
			      const boost::source_location& location) {
  std::string msg(std::make_error_code(ec).message());
  msg << ':' << location.file_name() << ':';
  ioutility::toChars(location.line(), msg);
  return msg << ' ' << location.function_name();
}

std::string createErrorString(const boost::source_location& location) {
  std::string msg(strerror(errno));
  msg << ':' << location.file_name() << ':';
  ioutility::toChars(location.line(), msg);
  return msg << ' ' << location.function_name();
}

} // end of namespace ioutility

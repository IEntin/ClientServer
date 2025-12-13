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
  buffer += toCharsBoost(width);
  buffer += 'x';
  unsigned height = std::get<1>(sizeKey);
  buffer += toCharsBoost(height);
  return buffer;
}

std::string createErrorString(std::errc ec,
			      const boost::source_location& location) {
  std::string msg(std::make_error_code(ec).message());
  msg << ':' << location.file_name() << ':';
  ioutility::toCharsBoost(location.line());
  return msg << ' ' << location.function_name();
}

std::string createErrorString(const boost::source_location& location) {
  std::string msg(strerror(errno));
  msg << ':' << location.file_name() << ':';
  msg += ioutility::toCharsBoost(location.line());
  return msg << ' ' << location.function_name();
}

bool processMessage(std::string_view payload,
		    HEADER& header,
		    std::span<std::reference_wrapper<std::string>> array) {
  if (payload.size() < HEADER_SIZE)
    return false;
  if (!deserialize(header, &payload[0]))
    return false;
  payload.remove_prefix(HEADER_SIZE);
  std::size_t sizes[] { extractField1Size(header), extractField2Size(header), extractField3Size(header) };
  unsigned shift = 0;
  for (unsigned i = 0; i < array.size(); ++i) {
    if (sizes[i] > 0) {
      array[i].get().assign(payload.cbegin() + shift, payload.cbegin() + shift + sizes[i]);
      shift += sizes[i];
    }
  }
  return true;
}

} // end of namespace ioutility

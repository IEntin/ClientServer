/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "IOUtility.h"

#include <mutex>
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
  if (!deserialize(header, payload.data()))
    return false;
  payload.remove_prefix(HEADER_SIZE);
  std::size_t sizes[] { extractField1Size(header),
			extractField2Size(header),
			extractField3Size(header),
			extractField4Size(header) };
  unsigned shift = 0;
  for (unsigned i = 0; i < array.size(); ++i) {
    if (sizes[i] > 0) {
      array[i].get().assign(payload.substr(shift, sizes[i]));
      shift += sizes[i];
    }
  }
  return true;
}

const boost::static_string<CONV_BUFFER_SIZE>& getRequestId(unsigned index) {
  static std::mutex mutex;
  std::unique_lock lock(mutex);
  static std::vector<boost::static_string<CONV_BUFFER_SIZE>> vector(10000);
  static bool initialized = false;
  if (!initialized) {
    for (unsigned i = 0; i < vector.size(); ++i) {
      vector[i] = { '[' };
      vector[i].append(toCharsBoost(i)).append(1, ']');
    }
    initialized = true;
  }
  return vector[index];
}

} // end of namespace ioutility

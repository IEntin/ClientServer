#include "Utility.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <cassert>

namespace utility {

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, std::string_view compressor) {
  std::memset(buffer, 0, HEADER_SIZE);
  bool ok = toChars(uncomprSz, buffer, NUM_FIELD_SIZE);
  assert(ok);
  ok = toChars(comprSz, buffer + NUM_FIELD_SIZE, NUM_FIELD_SIZE);
  assert(ok);
  std::strncpy(buffer + NUM_FIELD_SIZE * 2, compressor.data(), COMPRESSOR_NAME_SIZE - 1);
}

HEADER decodeHeader(std::string_view buffer, bool done) {
  size_t uncomprSize = 0;
  std::string_view stru(buffer.data(), NUM_FIELD_SIZE);
  if (!fromChars(stru, uncomprSize)) {
    return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false);
  }
  size_t comprSize = 0;
  std::string_view strc(buffer.data() + NUM_FIELD_SIZE, NUM_FIELD_SIZE);
  if (!fromChars(strc, comprSize)) {
    return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false);
  }
  std::string_view compressor(buffer.data() + NUM_FIELD_SIZE * 2, COMPRESSOR_NAME_SIZE);
  bool enabled = compressor.starts_with(LZ4);
  return std::make_tuple(uncomprSize, comprSize, enabled ? LZ4 : EMPTY_COMPRESSOR, done);
}

} // end of namespace utility

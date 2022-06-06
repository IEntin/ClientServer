/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"
#include<cassert>
#include<cstring>

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, COMPRESSORS compressor, bool diagnostics) {
  std::memset(buffer, 0, HEADER_SIZE);
  size_t offset = 0;
  bool ok = utility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
  assert(ok);
  offset += NUM_FIELD_SIZE;
  ok = utility::toChars(comprSz, buffer + offset, NUM_FIELD_SIZE);
  assert(ok);
  offset +=  NUM_FIELD_SIZE;
  ok = utility::toChars(static_cast<int>(compressor), buffer + offset, COMPRESSOR_TYPE_SIZE);
  assert(ok);
  offset += COMPRESSOR_TYPE_SIZE;
  buffer[offset] = (diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR);
}

HEADER decodeHeader(std::string_view buffer, bool done) {
  size_t offset = 0;
  size_t uncomprSize = 0;
  std::string_view stru(buffer.data(), NUM_FIELD_SIZE);
  if (!utility::fromChars(stru, uncomprSize))
    return { -1, -1, COMPRESSORS::NONE, false, false };
  offset += NUM_FIELD_SIZE;
  size_t comprSize = 0;
  std::string_view strc(buffer.data() + offset, NUM_FIELD_SIZE);
  if (!utility::fromChars(strc, comprSize))
    return { -1, -1, COMPRESSORS::NONE, false, false };
  offset += NUM_FIELD_SIZE;
  std::string_view str(buffer.data() + offset, COMPRESSOR_TYPE_SIZE);
  size_t value = 0;
  if (!utility::fromChars(str, value))
    return { -1, -1, COMPRESSORS::NONE, false, false };
  COMPRESSORS compressor = static_cast<COMPRESSORS>(value);
  offset += COMPRESSOR_TYPE_SIZE;
  bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
  return { uncomprSize, comprSize, compressor, diagnostics, done };
}

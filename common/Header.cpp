/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"
#include<cassert>
#include<cstring>

void encodeHeader(char* buffer,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS compressor,
		  bool diagnostics,
		  unsigned short ephemeral,
		  PROBLEMS problem) {
  std::memset(buffer, 0, HEADER_SIZE);
  size_t offset = 0;
  bool ok = utility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
  assert(ok);
  offset += NUM_FIELD_SIZE;
  ok = utility::toChars(comprSz, buffer + offset, NUM_FIELD_SIZE);
  assert(ok);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = static_cast<char>(compressor);
  offset += COMPRESSOR_TYPE_SIZE;
  buffer[offset] = (diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR);
  offset += DIAGNOSTICS_SIZE;
  ok = utility::toChars(ephemeral, buffer + offset, EPH_INDEX_SIZE);
  assert(ok);
  offset += EPH_INDEX_SIZE;
  buffer[offset] = static_cast<char>(problem);
}

HEADER decodeHeader(std::string_view buffer) {
  size_t offset = 0;
  size_t uncomprSize = 0;
  std::string_view stru(buffer.data(), NUM_FIELD_SIZE);
  if (!utility::fromChars(stru, uncomprSize))
    return { -1, -1, COMPRESSORS::NONE, false, 0, PROBLEMS::BAD_HEADER };
  offset += NUM_FIELD_SIZE;
  size_t comprSize = 0;
  std::string_view strc(buffer.data() + offset, NUM_FIELD_SIZE);
  if (!utility::fromChars(strc, comprSize))
    return { -1, -1, COMPRESSORS::NONE, false, 0, PROBLEMS::BAD_HEADER };
  offset += NUM_FIELD_SIZE;
  COMPRESSORS compressor = static_cast<COMPRESSORS>(buffer[offset]);
  offset += COMPRESSOR_TYPE_SIZE;
  bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  std::string_view streph(buffer.data() + offset, EPH_INDEX_SIZE);
  unsigned short ephemeral = 0;
  if (!utility::fromChars(streph, ephemeral))
    return { -1, -1, COMPRESSORS::NONE, false, 0, PROBLEMS::BAD_HEADER };
  offset += EPH_INDEX_SIZE;
  PROBLEMS problem = static_cast<PROBLEMS>(buffer[offset]);
  return { uncomprSize, comprSize, compressor, diagnostics, ephemeral, problem };
}

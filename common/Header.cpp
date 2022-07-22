/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"
#include<cassert>

void encodeHeader(char* buffer,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS compressor,
		  bool diagnostics,
		  unsigned short ephemeral,
		  unsigned short reserved,
		  PROBLEMS problem) {
  try {
    std::memset(buffer, 0, HEADER_SIZE);
    size_t offset = 0;
    utility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
    offset += NUM_FIELD_SIZE;
    utility::toChars(comprSz, buffer + offset, NUM_FIELD_SIZE);
    offset += NUM_FIELD_SIZE;
    buffer[offset] = std::underlying_type_t<COMPRESSORS>(compressor);
    offset += COMPRESSOR_TYPE_SIZE;
    buffer[offset] = (diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR);
    offset += DIAGNOSTICS_SIZE;
    utility::toChars(ephemeral, buffer + offset, EPH_INDEX_SIZE);
    offset += EPH_INDEX_SIZE;
    utility::toChars(reserved, buffer + offset, RESERVED_SIZE);
    offset += RESERVED_SIZE;
    buffer[offset] = std::underlying_type_t<PROBLEMS>(problem);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
}

HEADER decodeHeader(std::string_view buffer) {
  try {
    size_t offset = 0;
    size_t uncomprSize = 0;
    std::string_view stru(buffer.data(), NUM_FIELD_SIZE);
    utility::fromChars(stru, uncomprSize);
    offset += NUM_FIELD_SIZE;
    size_t comprSize = 0;
    std::string_view strc(buffer.data() + offset, NUM_FIELD_SIZE);
    utility::fromChars(strc, comprSize);
    offset += NUM_FIELD_SIZE;
    COMPRESSORS compressor = static_cast<COMPRESSORS>(buffer[offset]);
    offset += COMPRESSOR_TYPE_SIZE;
    bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
    offset += DIAGNOSTICS_SIZE;
    std::string_view streph(buffer.data() + offset, EPH_INDEX_SIZE);
    unsigned short ephemeral = 0;
    utility::fromChars(streph, ephemeral);
    offset += EPH_INDEX_SIZE;
    unsigned short reserved = 0;
    std::string_view strReserved(buffer.data() + offset, RESERVED_SIZE);
    utility::fromChars(strReserved, reserved);
    offset +=  RESERVED_SIZE;
    PROBLEMS problem = static_cast<PROBLEMS>(buffer[offset]);
    return { uncomprSize, comprSize, compressor, diagnostics, ephemeral, reserved, problem };
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    throw;
  }
}

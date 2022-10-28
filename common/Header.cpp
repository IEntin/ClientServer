/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"

void encodeHeader(char* buffer,
		  HEADERTYPE headerType,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS compressor,
		  bool diagnostics,
		  STATUS status) {
  try {
    std::memset(buffer, 0, HEADER_SIZE);
    size_t offset = 0;
    buffer[offset] = std::underlying_type_t<HEADERTYPE>(headerType);
    offset += HEADERTYPE_SIZE;
    utility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
    offset += NUM_FIELD_SIZE;
    utility::toChars(comprSz, buffer + offset, NUM_FIELD_SIZE);
    offset += NUM_FIELD_SIZE;
    buffer[offset] = std::underlying_type_t<COMPRESSORS>(compressor);
    offset += COMPRESSOR_TYPE_SIZE;
    buffer[offset] = (diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR);
    offset += DIAGNOSTICS_SIZE;
    buffer[offset] = std::underlying_type_t<STATUS>(status);
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
  }
}

HEADER decodeHeader(std::string_view buffer) {
  size_t offset = 0;
  HEADERTYPE headerType = static_cast<HEADERTYPE>(buffer[offset]);
  try {
    offset += HEADERTYPE_SIZE;
    size_t uncomprSize = 0;
    std::string_view stru(buffer.data() + offset, NUM_FIELD_SIZE);
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
    STATUS status = static_cast<STATUS>(buffer[offset]);
    return { headerType, uncomprSize, comprSize, compressor, diagnostics, status };
  }
  catch (const std::exception& e) {
    CERR << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << '\n';
    return { headerType, 0, 0, COMPRESSORS::NONE, false, STATUS::BAD_HEADER };
  }
}

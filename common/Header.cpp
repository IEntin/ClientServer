/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"

void encodeHeader(char* buffer,
		  HEADERTYPE headerType,
		  size_t payloadSz,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS compressor,
		  bool encrypted,
		  bool diagnostics,
		  STATUS status) {
  std::memset(buffer, 0, HEADER_SIZE);
  size_t offset = 0;
  buffer[offset] = std::underlying_type_t<HEADERTYPE>(headerType);
  offset += HEADERTYPE_SIZE;
  utility::toChars(payloadSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  utility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  utility::toChars(comprSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::underlying_type_t<COMPRESSORS>(compressor);
  offset += COMPRESSOR_TYPE_SIZE;
  buffer[offset] = (encrypted ? CRYPTO_CHAR : NCRYPTO_CHAR);
  offset += CRYPTO_TYPE_SIZE;
  buffer[offset] = (diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR);
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::underlying_type_t<STATUS>(status);
}

void encodeHeader(char* buffer, const HEADER& header) {
  auto [headerType, payloadSz, uncomprSz, comprSz, compressor, encrypted, diagnostics, status] = header;
  encodeHeader(buffer, headerType, payloadSz, uncomprSz, comprSz, compressor, encrypted, diagnostics, status);
}

HEADER decodeHeader(const char* buffer) {
  size_t offset = 0;
  HEADERTYPE headerType = static_cast<HEADERTYPE>(buffer[offset]);
  offset += HEADERTYPE_SIZE;
  size_t payloadSize = 0;
  std::string_view strt(buffer + offset, NUM_FIELD_SIZE);
  utility::fromChars(strt, payloadSize);
  offset += NUM_FIELD_SIZE;
  size_t uncomprSize = 0;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  utility::fromChars(stru, uncomprSize);
  offset += NUM_FIELD_SIZE;
  size_t comprSize = 0;
  std::string_view strc(buffer + offset, NUM_FIELD_SIZE);
  utility::fromChars(strc, comprSize);
  offset += NUM_FIELD_SIZE;
  COMPRESSORS compressor = static_cast<COMPRESSORS>(buffer[offset]);
  offset += COMPRESSOR_TYPE_SIZE;
  bool encrypted = buffer[offset] == CRYPTO_CHAR;
  offset += CRYPTO_TYPE_SIZE;
  bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  STATUS status = static_cast<STATUS>(buffer[offset]);
  return { headerType, payloadSize, uncomprSize, comprSize, compressor, encrypted, diagnostics, status };
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "IOUtility.h"
#include "Utility.h"

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::HEADERTYPE)>(header);
}

std::size_t extractPayloadSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::PAYLOADSIZE)>(header);
}

std::size_t extractUncompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::UNCOMPRESSED)>(header);
}

COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSOR)>(header);
}

bool isCompressed(const HEADER& header) {
  return extractCompressor(header) != COMPRESSORS::NONE;
}

bool isEncrypted(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::CRYPTO)>(header);
}

bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::DIAGNOSTICS)>(header);
}

STATUS extractStatus(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::STATUS)>(header);
}

std::size_t extractParameter(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::PARAMETER)>(header);
}

bool isOk(const HEADER& header) {
  STATUS status = extractStatus(header);
  switch(status) {
  case STATUS::NONE:
  case STATUS::MAX_TOTAL_OBJECTS:
  case STATUS::MAX_OBJECTS_OF_TYPE:
    return true;
  default:
    return false;
  }
}

void encodeHeader(char* buffer, const HEADER& header) {
  const auto& [headerType, payloadSz, uncomprSz, compressor, encrypted, diagnostics, status, parameter] = header;
  std::memset(buffer, 0, HEADER_SIZE);
  std::size_t offset = 0;
  buffer[offset] = std::underlying_type_t<HEADERTYPE>(headerType);
  offset += HEADERTYPE_SIZE;
  ioutility::toChars(payloadSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  ioutility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::underlying_type_t<COMPRESSORS>(compressor);
  offset += COMPRESSOR_SIZE;
  buffer[offset] = encrypted ? CRYPTO_CHAR : NCRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  buffer[offset] = diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::underlying_type_t<STATUS>(status);
  offset += STATUS_SIZE;
  ioutility::toChars(parameter, buffer + offset, PARAMETER_SIZE);
}

HEADER decodeHeader(const char* buffer) {
  std::size_t offset = 0;
  HEADERTYPE headerType = static_cast<HEADERTYPE>(buffer[offset]);
  offset += HEADERTYPE_SIZE;
  std::size_t payloadSize = 0;
  std::string_view strt(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(strt, payloadSize);
  offset += NUM_FIELD_SIZE;
  std::size_t uncomprSize = 0;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, uncomprSize);
  offset += NUM_FIELD_SIZE;
  COMPRESSORS compressor = static_cast<COMPRESSORS>(buffer[offset]);
  offset += COMPRESSOR_SIZE;
  bool encrypted = buffer[offset] == CRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  STATUS status = static_cast<STATUS>(buffer[offset]);
  offset += STATUS_SIZE;
  std::size_t parameter = 0;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, parameter);
  return { headerType, payloadSize, uncomprSize, compressor, encrypted, diagnostics, status, parameter };
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "ClientOptions.h"
#include "IOUtility.h"
#include "Logger.h"
#include "ServerOptions.h"

COMPRESSORS translateName(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::HEADERTYPEINDEX)>(header);
}

std::size_t extractPayloadSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header);
}

std::size_t extractUncompressedSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header);
}

COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::COMPRESSORINDEX)>(header);
}

bool isCompressed(const HEADER& header) {
  return extractCompressor(header) != COMPRESSORS::NONE;
}

bool isEncrypted(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::CRYPTOINDEX)>(header);
}

bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICSINDEX)>(header);
}

STATUS extractStatus(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::STATUSINDEX)>(header);
}

std::size_t extractParameter(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header);
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

void serialize(const HEADER& header, char* buffer) {
  std::memset(buffer, 0, HEADER_SIZE);
  std::size_t offset = 0;
  buffer[offset] = std::to_underlying(extractHeaderType(header));
  offset += HEADERTYPE_SIZE;
  ioutility::toChars(extractPayloadSize(header), buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  ioutility::toChars(extractUncompressedSize(header), buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::to_underlying(extractCompressor(header));
  offset += COMPRESSOR_SIZE;
  buffer[offset] = std::get<std::to_underlying(HEADER_INDEX::CRYPTOINDEX)>(header) ? CRYPTO_CHAR : NCRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  buffer[offset] = std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICSINDEX)>(header) ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::to_underlying(extractStatus(header));
  offset += STATUS_SIZE;
  ioutility::toChars(extractParameter(header), buffer + offset, PARAMETER_SIZE);
}

void deserialize(HEADER& header, const char* buffer) {
  std::size_t offset = 0;
  std::get<std::to_underlying(HEADER_INDEX::HEADERTYPEINDEX)>(header) =
    deserializeEnumeration(buffer[offset], HEADERTYPE::NONE, HEADERTYPE::INVALID);
  offset += HEADERTYPE_SIZE;
  std::string_view strl(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(strl, std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::COMPRESSORINDEX)>(header) =
    deserializeEnumeration(buffer[offset], COMPRESSORS::NONE, COMPRESSORS::INVALID);
  offset += COMPRESSOR_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::CRYPTOINDEX)>(header) = buffer[offset] == CRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICSINDEX)>(header) = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::STATUSINDEX)>(header) =
    deserializeEnumeration(buffer[offset], STATUS::NONE, STATUS::INVALID);
  offset += STATUS_SIZE;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header));
  if (ServerOptions::_printHeader || ClientOptions::_printHeader)
    printHeader(header);
}

void printHeader(const HEADER& header) {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << std::boolalpha;
  printTuple(header, logger);
}

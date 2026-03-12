/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"

#include "IOUtility.h"
#include "ServerOptions.h"

CRYPTO translateCryptoString(std::string_view cryptoStr) {
  if (cryptoStr == "CRYPTOPP")
    return CRYPTO::CRYPTOPP;
  else if (cryptoStr == "CRYPTOSODIUM")
    return CRYPTO::CRYPTOSODIUM;
  else if (cryptoStr == "NONE")
    return CRYPTO::NONE;
  else
    return CRYPTO::ERROR;
}

COMPRESSORS translateCompressorString(std::string_view compressorStr) {
  if (compressorStr == "LZ4")
    return COMPRESSORS::LZ4;
  else if (compressorStr == "SNAPPY")
    return COMPRESSORS::SNAPPY;
  else if (compressorStr == "ZSTD")
    return COMPRESSORS::ZSTD;
  else
    return COMPRESSORS::NONE;
}

DIAGNOSTICS translateDiagnosticsString(std::string_view diagnosticsStr) {
  DIAGNOSTICS diagnostics = diagnosticsStr == "Enabled" ? DIAGNOSTICS::ENABLED : DIAGNOSTICS::NONE;
  return diagnostics;
}

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<HEADERTYPE>(header);
}

std::size_t extractField1Size(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::FIELD1SIZEINDEX)>(header);
}

std::size_t extractField2Size(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::FIELD2SIZEINDEX)>(header);
}

COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<COMPRESSORS>(header);
}

bool isCompressed(const HEADER& header) {
  COMPRESSORS compressor = extractCompressor(header);
  switch (compressor) {
  case COMPRESSORS::LZ4:
  case COMPRESSORS::SNAPPY:
  case COMPRESSORS::ZSTD:
    return true;
  default:
    return false;
  }
}

DIAGNOSTICS extractDiagnostics(const HEADER& header) {
  return std::get<DIAGNOSTICS>(header);
}

bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<DIAGNOSTICS>(header) == DIAGNOSTICS::ENABLED;
}

STATUS extractStatus(const HEADER& header) {
  return std::get<STATUS>(header);
}

std::size_t extractField3Size(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::FIELD3SIZEINDEX)>(header);
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

boost::static_string<HEADER_SIZE> serialize(const HEADER& header) {
  serialized = std::to_underlying(extractHeaderType(header));
  serialized += ioutility::toCharsBoost(extractField1Size(header), true);
  serialized += ioutility::toCharsBoost(extractField2Size(header), true);
  serialized += std::to_underlying(extractCompressor(header));
  serialized += std::to_underlying(extractDiagnostics(header));
  serialized += std::to_underlying(extractStatus(header));
  serialized += ioutility::toCharsBoost(extractField3Size(header), true);
  return serialized;
}

bool deserialize(HEADER& header, const char* buffer) {
  std::size_t offset = 0;
  if (!deserializeEnumeration(std::get<HEADERTYPE>(header), buffer[offset]))
    return false;
  offset += HEADERTYPE_SIZE;
  std::string_view strz(buffer + offset, FIELD1_SIZE);
  ioutility::fromChars(strz, std::get<std::to_underlying(HEADER_INDEX::FIELD1SIZEINDEX)>(header));
  offset += FIELD1_SIZE;
  std::string_view stru(buffer + offset, FIELD2_SIZE);
  ioutility::fromChars(stru, std::get<std::to_underlying(HEADER_INDEX::FIELD2SIZEINDEX)>(header));
  offset += FIELD2_SIZE;
  if (!deserializeEnumeration(std::get<COMPRESSORS>(header), buffer[offset]))
    return false;
  offset += COMPRESSOR_SIZE;
  if (!deserializeEnumeration(std::get<DIAGNOSTICS>(header), buffer[offset]))
    return false;
  offset += DIAGNOSTICS_SIZE;
  if (!deserializeEnumeration(std::get<STATUS>(header), buffer[offset]))
    return false;
  offset += STATUS_SIZE;
  std::string_view strp(buffer + offset, FIELD3_SIZE);
  ioutility::fromChars(strp, std::get<std::to_underlying(HEADER_INDEX::FIELD3SIZEINDEX)>(header));
  if (ServerOptions::_printHeader)
    printHeader(header, LOG_LEVEL::ALWAYS);
  return true;
}

void printHeader(const HEADER& header, LOG_LEVEL level) {
  Logger logger(level, std::clog, false);
  logger << "\nheader:\n";
  logger << std::boolalpha;
  printTuple(header, logger);
}

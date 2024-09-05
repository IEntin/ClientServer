/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "ClientOptions.h"
#include "IOUtility.h"
#include "Logger.h"
#include "ServerOptions.h"

COMPRESSORS translateCompressorString(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

CRYPTO translateCryptoString(std::string_view cryptoStr) {
  CRYPTO crypto = cryptoStr == "Enabled" ? CRYPTO::ENCRYPTED : CRYPTO::NONE;
  return crypto;
}

DIAGNOSTICS translateDiagnosticsString(std::string_view diagnosticsStr) {
  DIAGNOSTICS diagnostics = diagnosticsStr == "Enabled" ? DIAGNOSTICS::ENABLED : DIAGNOSTICS::NONE;
  return diagnostics;
}

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<HEADERTYPE>(header);
}

std::size_t extractPayloadSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header);
}

std::size_t extractUncompressedSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header);
}

COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<COMPRESSORS>(header);
}

bool isCompressed(const HEADER& header) {
  return extractCompressor(header) == COMPRESSORS::LZ4;
}

CRYPTO extractCrypto(const HEADER& header) {
  return std::get<CRYPTO>(header);
}

bool isEncrypted(const HEADER& header) {
  return std::get<CRYPTO>(header) == CRYPTO::ENCRYPTED;
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
  buffer[offset] = std::to_underlying(extractCrypto(header));
  offset += CRYPTO_SIZE;
  buffer[offset] = std::to_underlying(extractDiagnostics(header));
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::to_underlying(extractStatus(header));
  offset += STATUS_SIZE;
  ioutility::toChars(extractParameter(header), buffer + offset, PARAMETER_SIZE);
}

bool deserialize(HEADER& header, const char* buffer) {
  std::size_t offset = 0;
  if (!deserializeEnumeration(std::get<HEADERTYPE>(header), buffer[offset]))
    return false;
  offset += HEADERTYPE_SIZE;
  std::string_view strl(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(strl, std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  if (!deserializeEnumeration(std::get<COMPRESSORS>(header), buffer[offset]))
    return false;
  offset += COMPRESSOR_SIZE;
  if (!deserializeEnumeration(std::get<CRYPTO>(header), buffer[offset]))
    return false;
  offset += CRYPTO_SIZE;
  if (!deserializeEnumeration(std::get<DIAGNOSTICS>(header), buffer[offset]))
    return false;
  offset += DIAGNOSTICS_SIZE;
  if (!deserializeEnumeration(std::get<STATUS>(header), buffer[offset]))
    return false;
  offset += STATUS_SIZE;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header));
  if (ServerOptions::_printHeader || ClientOptions::_printHeader)
    printHeader(header);
  return true;
}

void printHeader(const HEADER& header) {
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << std::boolalpha;
  printTuple(header, logger);
}

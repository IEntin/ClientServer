/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "IOUtility.h"
#include "Logger.h"
#include "Options.h"

COMPRESSORS translateCompressorString(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

DIAGNOSTICS translateDiagnosticsString(std::string_view diagnosticsStr) {
  DIAGNOSTICS diagnostics = diagnosticsStr == "Enabled" ? DIAGNOSTICS::ENABLED : DIAGNOSTICS::NONE;
  return diagnostics;
}

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<HEADERTYPE>(header);
}

unsigned extractReservedSz(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::RESERVEDINDEX)>(header);
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
  std::memset(buffer, ' ', HEADER_SIZE);
  std::size_t offset = 0;
  buffer[offset] = std::to_underlying(extractHeaderType(header));
  offset += HEADERTYPE_SIZE;
  ioutility::toChars(extractReservedSz(header), buffer + offset, RESERVED_SIZE);
  offset += RESERVED_SIZE;
  ioutility::toChars(extractUncompressedSize(header), buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::to_underlying(extractCompressor(header));
  offset += COMPRESSOR_SIZE;
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
  std::string_view strz(buffer + offset, RESERVED_SIZE);
  ioutility::fromChars(strz, std::get<std::to_underlying(HEADER_INDEX::RESERVEDINDEX)>(header));
  offset += RESERVED_SIZE;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  if (!deserializeEnumeration(std::get<COMPRESSORS>(header), buffer[offset]))
    return false;
  offset += COMPRESSOR_SIZE;
  if (!deserializeEnumeration(std::get<DIAGNOSTICS>(header), buffer[offset]))
    return false;
  offset += DIAGNOSTICS_SIZE;
  if (!deserializeEnumeration(std::get<STATUS>(header), buffer[offset]))
    return false;
  offset += STATUS_SIZE;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header));
  if (Options::_printHeader)
    printHeader(header, LOG_LEVEL::ALWAYS);
  return true;
}

void printHeader(const HEADER& header, LOG_LEVEL level) {
  Logger logger(level, std::clog, false);
  logger << "\nheader:\n";
  logger << std::boolalpha;
  printTuple(header, logger);
}

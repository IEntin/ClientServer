/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"

#include <utility>

#include "IOUtility.h"

COMPRESSORS translateName(std::string_view compressorStr) {
  COMPRESSORS compressor = compressorStr == "LZ4" ? COMPRESSORS::LZ4 : COMPRESSORS::NONE;
  return compressor;
}

HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::HEADERTYPEINDEX)>(header);
}

std::size_t extractPayloadSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZE)>(header);
}

std::size_t extractUncompressedSize(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSED)>(header);
}

COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::COMPRESSOR)>(header);
}

bool isCompressed(const HEADER& header) {
  return extractCompressor(header) != COMPRESSORS::NONE;
}

bool isEncrypted(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::CRYPTO)>(header);
}

bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICS)>(header);
}

STATUS extractStatus(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::STATUS)>(header);
}

std::size_t extractParameter(const HEADER& header) {
  return std::get<std::to_underlying(HEADER_INDEX::PARAMETER)>(header);
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
  buffer[offset] = std::to_underlying(headerType);
  offset += HEADERTYPE_SIZE;
  ioutility::toChars(payloadSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  ioutility::toChars(uncomprSz, buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::to_underlying(compressor);
  offset += COMPRESSOR_SIZE;
  buffer[offset] = encrypted ? CRYPTO_CHAR : NCRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  buffer[offset] = diagnostics ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::to_underlying(status);
  offset += STATUS_SIZE;
  ioutility::toChars(parameter, buffer + offset, PARAMETER_SIZE);
}

HEADER decodeHeader(const char* buffer) {
  std::size_t offset = 0;
  HEADERTYPE headerType = decodeHeadertype(buffer[offset]);
  offset += HEADERTYPE_SIZE;
  std::size_t payloadSize = 0;
  std::string_view strt(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(strt, payloadSize);
  offset += NUM_FIELD_SIZE;
  std::size_t uncomprSize = 0;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, uncomprSize);
  offset += NUM_FIELD_SIZE;
  COMPRESSORS compressor = decodeCompressor(buffer[offset]);
  offset += COMPRESSOR_SIZE;
  bool encrypted = buffer[offset] == CRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  bool diagnostics = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  STATUS status = decodeStatus(buffer[offset]);
  offset += STATUS_SIZE;
  std::size_t parameter = 0;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, parameter);
  return { headerType, payloadSize, uncomprSize, compressor, encrypted, diagnostics, status, parameter };
}

HEADERTYPE decodeHeadertype(uint8_t code) {
  switch (code) {
  case 0:
    return HEADERTYPE::CREATE_SESSION;
  case 1:
    return HEADERTYPE::SESSION;
  case 2:
    return HEADERTYPE::HEARTBEAT;
  case 3:
    return HEADERTYPE::ERROR;
  default:
    throw std::runtime_error("invalid HEADERTYPE");
  }
}

COMPRESSORS decodeCompressor(uint8_t code) {
  switch (code) {
  case 0:
    return COMPRESSORS::NONE;
  case 1:
    return COMPRESSORS::LZ4;
  default:
    throw std::runtime_error("invalid COMPRESSORS");
  }
}

STATUS decodeStatus(uint8_t code) {
  switch (code) {
  case 0:
    return STATUS::NONE;
  case 1:
    return STATUS::SUBTASK_DONE;
  case 2:
    return STATUS::TASK_DONE;
  case 3:
    return STATUS::BAD_HEADER;
  case 4:
    return STATUS::COMPRESSION_PROBLEM;
  case 5:
    return STATUS::ENCRYPTION_PROBLEM;
  case 6:
    return STATUS::DECRYPTION_PROBLEM;
  case 7:
    return STATUS::DECOMPRESSION_PROBLEM;
  case 8:
    return STATUS::FIFO_PROBLEM;
  case 9:
    return STATUS::TCP_PROBLEM;
  case 10:
    return STATUS::TCP_TIMEOUT;
  case 11:
    return STATUS::HEARTBEAT_PROBLEM;
  case 12:
    return STATUS::HEARTBEAT_TIMEOUT;
  case 13:
    return STATUS::MAX_TOTAL_OBJECTS;
  case 14:
    return STATUS::MAX_OBJECTS_OF_TYPE;
  case 15:
    return STATUS::STOPPED;
  case 16:
    return STATUS::ERROR;
  default:
    throw std::runtime_error("invalid STATUS");
  }
}

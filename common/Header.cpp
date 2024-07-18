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
  case std::to_underlying(HEADERTYPE::CREATE_SESSION):
    return HEADERTYPE::CREATE_SESSION;
  case std::to_underlying(HEADERTYPE::SESSION):
    return HEADERTYPE::SESSION;
  case std::to_underlying(HEADERTYPE::HEARTBEAT):
    return HEADERTYPE::HEARTBEAT;
  case std::to_underlying(HEADERTYPE::ERROR):
    return HEADERTYPE::ERROR;
  default:
    throw std::runtime_error("invalid HEADERTYPE");
  }
}

COMPRESSORS decodeCompressor(uint8_t code) {
  switch (code) {
  case std::to_underlying(COMPRESSORS::NONE):
    return COMPRESSORS::NONE;
  case std::to_underlying(COMPRESSORS::LZ4):
    return COMPRESSORS::LZ4;
  default:
    throw std::runtime_error("invalid COMPRESSORS");
  }
}

STATUS decodeStatus(uint8_t code) {
  switch (code) {
  case std::to_underlying(STATUS::NONE):
    return STATUS::NONE;
  case std::to_underlying(STATUS::SUBTASK_DONE):
    return STATUS::SUBTASK_DONE;
  case std::to_underlying(STATUS::TASK_DONE):
    return STATUS::TASK_DONE;
  case std::to_underlying(STATUS::BAD_HEADER):
    return STATUS::BAD_HEADER;
  case std::to_underlying(STATUS::COMPRESSION_PROBLEM):
    return STATUS::COMPRESSION_PROBLEM;
  case std::to_underlying(STATUS::ENCRYPTION_PROBLEM):
    return STATUS::ENCRYPTION_PROBLEM;
  case std::to_underlying(STATUS::DECRYPTION_PROBLEM):
    return STATUS::DECRYPTION_PROBLEM;
  case std::to_underlying(STATUS::DECOMPRESSION_PROBLEM):
    return STATUS::DECOMPRESSION_PROBLEM;
  case std::to_underlying(STATUS::FIFO_PROBLEM):
    return STATUS::FIFO_PROBLEM;
  case std::to_underlying(STATUS::TCP_PROBLEM):
    return STATUS::TCP_PROBLEM;
  case std::to_underlying(STATUS::TCP_TIMEOUT):
    return STATUS::TCP_TIMEOUT;
  case std::to_underlying(STATUS::HEARTBEAT_PROBLEM):
    return STATUS::HEARTBEAT_PROBLEM;
  case std::to_underlying(STATUS::HEARTBEAT_TIMEOUT):
    return STATUS::HEARTBEAT_TIMEOUT;
  case std::to_underlying(STATUS::MAX_TOTAL_OBJECTS):
    return STATUS::MAX_TOTAL_OBJECTS;
  case std::to_underlying(STATUS::MAX_OBJECTS_OF_TYPE):
    return STATUS::MAX_OBJECTS_OF_TYPE;
  case std::to_underlying(STATUS::STOPPED):
    return STATUS::STOPPED;
  case std::to_underlying(STATUS::ERROR):
    return STATUS::ERROR;
  default:
    throw std::runtime_error("invalid STATUS");
  }
}

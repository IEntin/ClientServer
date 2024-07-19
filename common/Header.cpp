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
  buffer[offset] = std::to_underlying(std::get<std::to_underlying(HEADER_INDEX::HEADERTYPEINDEX)>(header));
  offset += HEADERTYPE_SIZE;
  ioutility::toChars(std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header), buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  ioutility::toChars(std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header), buffer + offset, NUM_FIELD_SIZE);
  offset += NUM_FIELD_SIZE;
  buffer[offset] = std::to_underlying(std::get<std::to_underlying(HEADER_INDEX::COMPRESSORINDEX)>(header));
  offset += COMPRESSOR_SIZE;
  buffer[offset] = std::get<std::to_underlying(HEADER_INDEX::CRYPTOINDEX)>(header) ? CRYPTO_CHAR : NCRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  buffer[offset] = std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICSINDEX)>(header) ? DIAGNOSTICS_CHAR : NDIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  buffer[offset] = std::to_underlying(std::get<std::to_underlying(HEADER_INDEX::STATUSINDEX)>(header));
  offset += STATUS_SIZE;
  ioutility::toChars(std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header), buffer + offset, PARAMETER_SIZE);
}

void deserialize(HEADER& header, const char* buffer) {
  std::size_t offset = 0;
  std::get<std::to_underlying(HEADER_INDEX::HEADERTYPEINDEX)>(header) =
    deserializeHeadertype(buffer[offset]);
  offset += HEADERTYPE_SIZE;
  std::string_view strl(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(strl, std::get<std::to_underlying(HEADER_INDEX::PAYLOADSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  std::string_view stru(buffer + offset, NUM_FIELD_SIZE);
  ioutility::fromChars(stru, std::get<std::to_underlying(HEADER_INDEX::UNCOMPRESSEDSIZEINDEX)>(header));
  offset += NUM_FIELD_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::COMPRESSORINDEX)>(header) = deserializeCompressor(buffer[offset]);
  offset += COMPRESSOR_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::CRYPTOINDEX)>(header) = buffer[offset] == CRYPTO_CHAR;
  offset += CRYPTO_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::DIAGNOSTICSINDEX)>(header) = buffer[offset] == DIAGNOSTICS_CHAR;
  offset += DIAGNOSTICS_SIZE;
  std::get<std::to_underlying(HEADER_INDEX::STATUSINDEX)>(header) = deserializeStatus(buffer[offset]);
  offset += STATUS_SIZE;
  std::string_view strp(buffer + offset, PARAMETER_SIZE);
  ioutility::fromChars(strp, std::get<std::to_underlying(HEADER_INDEX::PARAMETERINDEX)>(header));
}

HEADERTYPE deserializeHeadertype(std::uint8_t code) {
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

COMPRESSORS deserializeCompressor(std::uint8_t code) {
  switch (code) {
  case std::to_underlying(COMPRESSORS::NONE):
    return COMPRESSORS::NONE;
  case std::to_underlying(COMPRESSORS::LZ4):
    return COMPRESSORS::LZ4;
  default:
    throw std::runtime_error("invalid COMPRESSORS");
  }
}

STATUS deserializeStatus(std::uint8_t code) {
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

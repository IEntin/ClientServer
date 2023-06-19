/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <sys/types.h>
#include <tuple>

inline constexpr int HEADERTYPE_SIZE = 1;
inline constexpr int NUM_FIELD_SIZE = 10;
inline constexpr int COMPRESSOR_TYPE_SIZE = 1;
inline constexpr int CRYPTO_TYPE_SIZE = 1;
inline constexpr int DIAGNOSTICS_SIZE = 1;
inline constexpr int STATUS_SIZE = 1;
inline constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + CRYPTO_TYPE_SIZE + DIAGNOSTICS_SIZE + STATUS_SIZE;

inline constexpr char CRYPTO_CHAR = 'C';
inline constexpr char NCRYPTO_CHAR = 'N';

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class HEADERTYPE : char {
  CREATE_SESSION,
  SESSION,
  HEARTBEAT,
  ERROR
};

enum class COMPRESSORS : char {
  NONE,
  LZ4
};

enum class STATUS : char {
  NONE,
  SUBTASK_DONE,
  TASK_DONE,
  BAD_HEADER,
  COMPRESSION_PROBLEM,
  ENCRYPTION_PROBLEM,
  DECRYPTION_PROBLEM,
  DECOMPRESSION_PROBLEM,
  FIFO_PROBLEM,
  TCP_PROBLEM,
  TCP_TIMEOUT,
  HEARTBEAT_PROBLEM,
  HEARTBEAT_TIMEOUT,
  MAX_TOTAL_OBJECTS,
  MAX_OBJECTS_OF_TYPE,
  ERROR
};

enum class ACTIONS : int {
  NONE,
  ACTION,
  STOP
};

enum class HEADER_INDEX : char {
  HEADERTYPE,
  UNCOMPRESSED,
  COMPRESSED,
  COMPRESSOR,
  CRYPTO,
  DIAGNOSTICS,
  STATUS
};

using HEADER = std::tuple<HEADERTYPE, size_t, size_t, COMPRESSORS, bool, bool, STATUS>;

inline HEADERTYPE extractHeaderType(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::HEADERTYPE)>(header);
}

inline ssize_t extractUncompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::UNCOMPRESSED)>(header);
}

inline ssize_t extractCompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSED)>(header);
}

inline COMPRESSORS extractCompressor(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSOR)>(header);
}

inline bool isCompressed(const HEADER& header) {
  return extractCompressor(header) != COMPRESSORS::NONE;
}

inline bool isEncrypted(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::CRYPTO)>(header);
}

inline bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::DIAGNOSTICS)>(header);
}

inline STATUS extractStatus(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::STATUS)>(header);
}

inline bool isOk(const HEADER& header) {
  STATUS status = extractStatus(header);
  switch(status) {
  case STATUS::NONE:
  case STATUS::MAX_TOTAL_OBJECTS:
  case STATUS::MAX_OBJECTS_OF_TYPE:
    return true;
    break;
  default:
    return false;
    break;
  }
}

void encodeHeader(char* buffer,
		  HEADERTYPE headerType,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS,
		  bool encrypted,
		  bool diagnostics,
		  STATUS status = STATUS::NONE);

void encodeHeader(char* buffer, const HEADER& header);

HEADER decodeHeader(const char* buffer);

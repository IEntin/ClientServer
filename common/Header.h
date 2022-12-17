/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

inline constexpr int HEADERTYPE_SIZE = 1;
inline constexpr int NUM_FIELD_SIZE = 10;
inline constexpr int COMPRESSOR_TYPE_SIZE = 1;
inline constexpr int DIAGNOSTICS_SIZE = 1;
inline constexpr int STATUS_SIZE = 1;
inline constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + DIAGNOSTICS_SIZE + STATUS_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class HEADERTYPE : char {
  CREATE_SESSION,
  SESSION,
  DESTROY_SESSION,
  CREATE_HEARTBEAT,
  HEARTBEAT,
  DESTROY_HEARTBEAT,
  ERROR
};

enum class COMPRESSORS : char {
  NONE,
  LZ4
};

enum class STATUS : char {
  NONE,
  BAD_HEADER,
  COMPRESSION_PROBLEM,
  DECOMPRESSION_PROBLEM,
  FIFO_PROBLEM,
  TCP_PROBLEM,
  TCP_TIMEOUT,
  HEARTBEAT_PROBLEM,
  HEARTBEAT_TIMEOUT,
  MAX_TOTAL_SESSIONS,
  MAX_SPECIFIC_SESSIONS
};

enum class HEADER_INDEX : char {
  HEADERTYPE,
  UNCOMPRESSED,
  COMPRESSED,
  COMPRESSOR,
  DIAGNOSTICS,
  STATUS
};

using HEADER = std::tuple<HEADERTYPE, size_t, size_t, COMPRESSORS, bool, STATUS>;

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

inline bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::DIAGNOSTICS)>(header);
}

inline STATUS extractStatus(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::STATUS)>(header);
}

inline bool isOk(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::STATUS)>(header) == STATUS::NONE;
}

void encodeHeader(char* buffer,
		  HEADERTYPE headerType,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS,
		  bool diagnostics,
		  STATUS status = STATUS::NONE);

void encodeHeader(char* buffer, const HEADER& header);

HEADER decodeHeader(const char* buffer);

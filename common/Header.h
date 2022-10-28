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
inline constexpr int PROBLEM_SIZE = 1;
inline constexpr int HEADER_SIZE =
  HEADERTYPE_SIZE + NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + DIAGNOSTICS_SIZE + PROBLEM_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';
inline constexpr char HEADERTYPE_REQUEST_CHAR = 'R';
inline constexpr char HEADERTYPE_HEARTBEAT_CHAR = 'H';
inline constexpr char HEADERTYPE_CLIENTID_CHAR = 'C';

enum class SESSIONTYPE : char { SESSION = HEADERTYPE_REQUEST_CHAR,
			     HEARTBEAT = HEADERTYPE_HEARTBEAT_CHAR,
			     CLIENTID = HEADERTYPE_CLIENTID_CHAR };

enum class HEADERTYPE : char {
  REQUEST = HEADERTYPE_REQUEST_CHAR,
  HEARTBEAT = HEADERTYPE_HEARTBEAT_CHAR,
  CLIENTID = HEADERTYPE_CLIENTID_CHAR
};

enum class COMPRESSORS : char {
  NONE,
  LZ4
};

enum class STATUS : char {
  NONE,
  BAD_HEADER,
  FIFO_PROBLEM,
  TCP_PROBLEM,
  TCP_TIMEOUT,
  MAX_TOTAL_SESSIONS,
  MAX_NUMBER_RUNNABLES
};

enum class HEADER_INDEX : int {
  HEADERTYPE,
  UNCOMPRESSED,
  COMPRESSED,
  COMPRESSOR,
  DIAGNOSTICS,
  STATUS
};

constexpr unsigned short HSMSG_SIZE = 50;

enum class HSMSG_INDEX : int {
  TYPE,
  ID
};

using HEADER = std::tuple<HEADERTYPE, size_t, size_t, COMPRESSORS, bool, STATUS>;

inline HEADERTYPE getHeaderType(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::HEADERTYPE)>(header);
}

inline ssize_t getUncompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::UNCOMPRESSED)>(header);
}

inline ssize_t getCompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSED)>(header);
}

inline COMPRESSORS getCompressor(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSOR)>(header);
}

inline bool isInputCompressed(const HEADER& header) {
  return getCompressor(header) == COMPRESSORS::LZ4;
}

inline bool isDiagnosticsEnabled(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::DIAGNOSTICS)>(header);
}

inline STATUS getProblem(const HEADER& header) {
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

HEADER decodeHeader(std::string_view buffer);

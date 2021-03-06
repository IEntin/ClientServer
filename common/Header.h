/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

inline constexpr int NUM_FIELD_SIZE = 10;
inline constexpr int COMPRESSOR_TYPE_SIZE = 1;
inline constexpr int DIAGNOSTICS_SIZE = 1;
inline constexpr int EPH_INDEX_SIZE = 5;
inline constexpr int RESERVED_SIZE = 5;
inline constexpr int PROBLEM_SIZE = 1;
inline constexpr int HEADER_SIZE =
  NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + DIAGNOSTICS_SIZE + EPH_INDEX_SIZE + RESERVED_SIZE + PROBLEM_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class COMPRESSORS : char {
  NONE,
  LZ4
};

enum class PROBLEMS : char {
  NONE,
  BAD_HEADER,
  MAX_FIFO_SESSIONS,
  FIFO_PROBLEM,
  MAX_TCP_SESSIONS,
  TCP_PROBLEM,
  MAX_TOTAL_SESSIONS
};

enum class HEADER_INDEX : int {
  UNCOMPRESSED_SIZE,
  COMPRESSED_SIZE,
  COMPRESSOR,
  DIAGNOSTICS,
  EPHEMERAL,
  RESERVED,
  PROBLEMS
};

using HEADER = std::tuple<size_t, size_t, COMPRESSORS, bool, unsigned short, unsigned short, PROBLEMS>;

inline ssize_t getUncompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::UNCOMPRESSED_SIZE)>(header);
}

inline ssize_t getCompressedSize(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::COMPRESSED_SIZE)>(header);
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

inline unsigned short getEphemeral(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::EPHEMERAL)>(header);
}

inline unsigned short getReserved(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::RESERVED)>(header);
}

inline PROBLEMS getProblem(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::PROBLEMS)>(header);
}

inline bool isOk(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::PROBLEMS)>(header) == PROBLEMS::NONE;
}

void encodeHeader(char* buffer,
		  size_t uncomprSz,
		  size_t comprSz,
		  COMPRESSORS,
		  bool diagnostics,
		  unsigned short ephemeral,
		  unsigned short reserved,
		  PROBLEMS = PROBLEMS::NONE);

HEADER decodeHeader(std::string_view buffer);

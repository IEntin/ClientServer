/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

inline constexpr int NUM_FIELD_SIZE = 10;
inline constexpr int COMPRESSOR_TYPE_SIZE = 2;
inline constexpr int DIAGNOSTICS_SIZE = 2;
inline constexpr int UNUSED_SIZE = 4;
inline constexpr int HEADER_SIZE = NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + DIAGNOSTICS_SIZE + UNUSED_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class COMPRESSORS : short {
  NONE,
  LZ4
};

enum class HEADER_INDEX : short {
  UNCOMPRESSED_SIZE,
  COMPRESSED_SIZE,
  COMPRESSOR,
  DIAGNOSTICS,
  DONE
};

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

inline bool isOk(const HEADER& header) {
  return std::get<static_cast<int>(HEADER_INDEX::DONE)>(header);
}

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

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, COMPRESSORS, bool diagnostics);

HEADER decodeHeader(std::string_view buffer, bool done = true);

#pragma once

#include <string_view>
#include <sys/types.h>
#include <tuple>

inline constexpr size_t NUM_FIELD_SIZE = 10;
inline constexpr size_t COMPRESSOR_TYPE_SIZE = 2;
inline constexpr size_t DIAGNOSTICS_SIZE = 2;
inline constexpr size_t UNUSED_SIZE = 4;
inline constexpr size_t HEADER_SIZE = NUM_FIELD_SIZE * 2 + COMPRESSOR_TYPE_SIZE + DIAGNOSTICS_SIZE + UNUSED_SIZE;

inline constexpr char DIAGNOSTICS_CHAR = 'D';
inline constexpr char NDIAGNOSTICS_CHAR = 'N';

enum class COMPRESSORS : short unsigned int {
  NONE,
  LZ4
};

enum class HEADER_INDEX : unsigned {
  UNCOMPRESSED_SIZE,
  COMPRESSED_SIZE,
  COMPRESSOR,
  DIAGNOSTICS,
  DONE
};

using HEADER = std::tuple<ssize_t, ssize_t, COMPRESSORS, bool, bool>;

inline ssize_t getUncompressedSize(const HEADER& header) {
  return std::get<static_cast<unsigned>(HEADER_INDEX::UNCOMPRESSED_SIZE)>(header);
}

inline ssize_t getCompressedSize(const HEADER& header) {
  return std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSED_SIZE)>(header);
}

inline COMPRESSORS getCompressor(const HEADER& header) {
  return std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSOR)>(header);
}

inline bool getDiagnostics(const HEADER& header) {
  return std::get<static_cast<unsigned>(HEADER_INDEX::DIAGNOSTICS)>(header);
}

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, COMPRESSORS, bool diagnostics);

HEADER decodeHeader(std::string_view buffer, bool done = true);
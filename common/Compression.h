/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

inline constexpr std::string_view LZ4 = "LZ4";

enum class COMPRESSORS : char;

class Compression {
  Compression() = delete;
  ~Compression() = delete;
 public:
  static COMPRESSORS isCompressionEnabled(const std::string& compressorStr);

  static std::string_view compress(const char* uncompressed, size_t uncompressedSize);

  static std::string_view uncompress(const char* compressed,
				     size_t compressedSize,
				     size_t uncomprSize);

  static bool uncompress(const std::vector<char>& compressed,
			 size_t compressedSize,
			 std::vector<char>& uncompressed);

  static int getCompressBound(int uncomprSize);
};

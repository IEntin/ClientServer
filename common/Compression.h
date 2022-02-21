/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

inline constexpr std::string_view LZ4 = "LZ4";
inline constexpr std::string_view NOP = "NOP";

enum class COMPRESSORS : unsigned short;

using CompressionDescription = std::pair<COMPRESSORS, bool>;

class Compression {
  Compression() = delete;
  ~Compression() = delete;

  static std::string_view compressInternal(std::string_view uncompressed,
					   char* buffer,
					   size_t dstCapacity);
  static bool uncompressInternal(std::string_view compressed,
				 char* uncompressed,
				 size_t uncomprSize);
 public:
  static CompressionDescription isCompressionEnabled(const std::string& compressorStr);

  static std::string_view compress(std::string_view origin);

  static std::string_view uncompress(std::string_view compressed, size_t uncomprSize);

  static bool uncompress(std::string_view compressed, std::vector<char>& uncompressed);

  static size_t getCompressBound(size_t uncomprSize);
};

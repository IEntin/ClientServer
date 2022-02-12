/*
 *  Copyright (C) 2021 Ilya Entin
 */

#pragma once

#include <string>
#include <vector>

inline constexpr std::string_view LZ4 = "LZ4";
inline constexpr std::string_view NOP = "NOP";

enum class COMPRESSORS : unsigned short;

class Compression {
  Compression() = delete;
  ~Compression() = delete;

  static std::string_view compressInternal(std::string_view uncompressed,
					   char* buffer,
					   size_t dstCapacity);
  static bool uncompressInternal(std::string_view compressed,
				 char* uncompressed,
				 size_t uncomprSize);

  static COMPRESSORS _compressor;
  static bool _enabled;
 public:
  static bool setCompressionEnabled(const std::string& compressorStr);

  static std::pair<COMPRESSORS, bool> isCompressionEnabled();

  static std::string_view compress(std::string_view origin);

  static std::string_view uncompress(std::string_view compressed, size_t uncomprSize);

  static bool uncompress(std::string_view compressed, std::vector<char>& uncompressed);

  static size_t getCompressBound(size_t uncomprSize);
};

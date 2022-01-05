#pragma once

#include <string>
#include <vector>

inline constexpr std::string_view EMPTY_COMPRESSOR = "EMP";
inline constexpr std::string_view LZ4 = "LZ4";

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
  static std::pair<std::string_view, bool> isCompressionEnabled();

  static std::string_view compress(std::string_view origin);

  static std::string_view uncompress(std::string_view compressed, size_t uncomprSize);

  static bool uncompress(std::string_view compressed, std::vector<char>& uncompressed);

  static size_t getCompressBound(size_t uncomprSize);
};

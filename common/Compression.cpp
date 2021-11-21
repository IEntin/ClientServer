#include "Compression.h"
#include "MemoryPool.h"
#include "ProgramOptions.h"
#include <iostream>
#include "lz4.h"

std::pair<std::string_view, bool> Compression::isCompressionEnabled() {
  static std::string compressor = ProgramOptions::get("Compression", std::string());
  static bool enabled = compressor.starts_with("LZ4");
  static auto& dummy[[maybe_unused]] =
    std::clog << LZ4 << "compression " << (enabled ? "enabled" : "disabled") << std::endl;
  return enabled ? std::make_pair(LZ4, true) : std::make_pair(EMPTY_COMPRESSOR, false);
}

std::string_view Compression::compressInternal(std::string_view uncompressed,
					       char* buffer,
					       size_t dstCapacity) {
  size_t compressedSize = LZ4_compress_default(uncompressed.data(),
					       buffer,
					       uncompressed.size(),
					       dstCapacity);
  if (compressedSize == 0) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " failed" << std::endl;
    return { nullptr, 0 };
  }
  return { buffer, compressedSize };
}

bool Compression::uncompressInternal(std::string_view compressed,
				     char* uncompressed,
				     size_t uncomprSize) {
  ssize_t decomprSize = LZ4_decompress_safe(compressed.data(),
					    uncompressed,
					    compressed.size(),
					    uncomprSize);
  if (decomprSize != static_cast<ssize_t>(uncomprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << " failed" << std::endl;
    return false;
  }
  return true;
}

size_t Compression::getCompressBound(size_t uncomprSize) {
  return LZ4_compressBound(uncomprSize);
}

std::string_view Compression::compress(std::string_view origin) {
  size_t dstCapacity = LZ4_compressBound(origin.size());
  auto[buffer, size] = MemoryPool::getPrimaryBuffer(dstCapacity);
  assert(buffer);
  std::string_view dstView = compressInternal(origin, buffer, dstCapacity);
  if (dstView.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to compress payload" << std::endl;
    return { nullptr, 0 };
  }
  return dstView;
}

std::string_view Compression::uncompress(std::string_view compressed, size_t uncomprSize) {
  auto[buffer, size] = MemoryPool::getPrimaryBuffer(uncomprSize);
  assert(buffer);
  if (!uncompressInternal(compressed, buffer, uncomprSize)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return { nullptr, 0 };
  }
  return { buffer, uncomprSize };
}

bool Compression::uncompress(std::string_view compressed, std::vector<char>& uncompressed) {
  if (!uncompressInternal(compressed, &uncompressed[0], uncompressed.size())) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << " failed" << std::endl;
    return false;
  }
  return true;
}

bool Compression::testCompressionDecompression(std::string_view input) {
  static const std::string stringTypeInTask = ProgramOptions::get("StringTypeInTask", std::string());
  static const bool useString = stringTypeInTask == "STRING";
  if (useString) {
    std::string_view compressedView = compress(input);
    if (compressedView.empty())
      return false;
    // save in a string before buffer is reused in uncompress
    std::string compressed;
    compressed.assign(compressedView.data(), compressedView.size());
    // saving not needed, it is the final step
    std::string_view uncompressedView = uncompress(compressed, input.size());
    std::clog << "   input.size()=" << input.size() << " compressedView.size()="
	      << compressedView.size() << " restored to original:" << std::boolalpha
	      << (input == uncompressedView) << std::endl;
    return input == uncompressedView;
  }
  else {
    std::string_view compressedView = compress(input);
    if (compressedView.empty())
      return false;
    std::vector<char> uncompressed(input.size());
    if (!uncompress(compressedView, uncompressed))
      return false;
    std::string_view uncompressedView(uncompressed.begin(), uncompressed.end());
    std::clog << "   input.size()=" << input.size() << " compressedView.size()="
	      << compressedView.size() << " restored to original:" << std::boolalpha
	      << (input == uncompressedView) << std::endl;
    return input == uncompressedView;
  }
}

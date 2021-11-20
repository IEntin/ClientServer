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

std::string_view Compression::compress(std::string_view origin, std::string& compressed) {
  size_t dstCapacity = LZ4_compressBound(origin.size());
  auto[buffer, size] = MemoryPool::getBuffer(dstCapacity);
  if (buffer) {
    std::string_view dstView = compressInternal(origin, buffer, dstCapacity);
    if (dstView.empty()) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< ":failed to compress payload" << std::endl;
      return { nullptr, 0 };
    }
    return dstView;
  }
  static bool enableHints = ProgramOptions::get("EnableHints", true);
  if (enableHints)
    static auto& dummy[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__
      << ' ' << __func__ << "-server and some clients have different "
      "buffer sizes and memory pooling is disabled.Set \"EnableHints\" "
      "to false to disable this message." << std::endl;
  // buffer was not found in the pool. allocate here
  std::vector<char> localBuffer(dstCapacity);
  std::string_view dstView = compressInternal(origin, &localBuffer[0], dstCapacity);
  // save the result in caller space
  compressed.assign(dstView);
  if (compressed.empty()) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to compress payload" << std::endl;
    return { nullptr, 0 };
  }
  return { compressed.data(), compressed.size() };
}

std::string_view Compression::uncompress(std::string_view compressed,
					 size_t uncomprSize,
					 std::string& uncompressed) {
  auto[buffer, size] = MemoryPool::getBuffer(uncomprSize);
  if (buffer) {
    if (!uncompressInternal(compressed, buffer, uncomprSize)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< " failed" << std::endl;
      return { nullptr, 0 };
    }
    return { buffer, uncomprSize };
  }
  else {
    static bool enableHints = ProgramOptions::get("EnableHints", true);
    if (enableHints)
      static auto& dummy[[maybe_unused]] = std::clog << __FILE__ << ':' << __LINE__
	<< ' ' << __func__ << "-Server and some clients have different "
	"buffer sizes and memory pooling is disabled.Set \"EnableHints\" "
	"to false to disable this message." << std::endl;
    // buffer was not found in the pool. allocate here
    std::vector<char> buffer(uncomprSize);
    if (!uncompressInternal(compressed, &buffer[0], uncomprSize)) {
      std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
		<< " failed" << std::endl;
      return { nullptr, 0 };
    }
    // save the result in caller space
    uncompressed.assign(&buffer[0], uncomprSize);
    return { uncompressed.data(), uncomprSize };
  }
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
    std::string compressed;
    std::string_view compressedView = compress(input, compressed);
    if (compressedView.empty())
      return false;
    // save in a string before buffer is reused in uncompress
    compressed.assign(compressedView.data(), compressedView.size());
    std::string uncompressed;
    // no saving here, it is the final step
    std::string_view uncompressedView = uncompress(compressed, input.size(), uncompressed);
    std::clog << "   input.size()=" << input.size() << " compressedView.size()="
	      << compressedView.size() << " restored to original:" << std::boolalpha
	      << (input == uncompressedView) << std::endl;
    return input == uncompressedView;
  }
  else {
    std::string compressed;
    std::string_view compressedView = compress(input, compressed);
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

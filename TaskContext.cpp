#include "TaskContext.h"
#include "Compression.h"
#include <iostream>

TaskContext::TaskContext(const HEADER& header) :
  _header(header),
  _inputCompressed(getCompressor(_header) == COMPRESSORS::LZ4),
  _compressedSize(getCompressedSize(_header)),
  _uncompressedSize(getUncompressedSize(_header)),
  _diagnostics(isDiagnosticsEnabled(_header)) {}

bool TaskContext::decompress(const std::vector<char>& input, std::vector<char>& uncompressed) {
  std::string_view received(input.data(), input.size());
  uncompressed.resize(_uncompressedSize);
  if (!Compression::uncompress(received, uncompressed)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << ":failed to uncompress payload" << std::endl;
    return false;
  }
  return true;
}

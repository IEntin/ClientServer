#include "TaskContext.h"
#include "Compression.h"

TaskContext::TaskContext(std::string_view headerView) :
  _header(utility::decodeHeader(headerView)),
  _inputCompressed(std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSOR)>(_header).starts_with(LZ4)),
  _compressedSize(std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSED_SIZE)>(_header)),
  _uncompressedSize(std::get<static_cast<unsigned>(HEADER_INDEX::UNCOMPRESSED_SIZE)>(_header)),
  _diagnostics(std::get<static_cast<unsigned>(HEADER_INDEX::DIAGNOSTICS)>(_header))  {}

TaskContext::TaskContext(const HEADER& header) :
  _header(header),
  _inputCompressed(std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSOR)>(_header).starts_with(LZ4)),
  _compressedSize(std::get<static_cast<unsigned>(HEADER_INDEX::COMPRESSED_SIZE)>(_header)),
  _uncompressedSize(std::get<static_cast<unsigned>(HEADER_INDEX::UNCOMPRESSED_SIZE)>(_header)),
  _diagnostics(std::get<static_cast<unsigned>(HEADER_INDEX::DIAGNOSTICS)>(_header)) {}

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

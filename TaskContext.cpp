#include "TaskContext.h"
#include "Compression.h"

TaskContext::TaskContext(std::string_view headerView) :
  _header(utility::decodeHeader(headerView)),
  _inputCompressed(std::get<2>(_header).starts_with(LZ4)),
  _compressedSize(std::get<1>(_header)),
  _uncompressedSize(std::get<0>(_header)),
  _diagnostics(std::get<3>(_header))  {}

TaskContext::TaskContext(const HEADER& header) :
  _header(header),
  _inputCompressed(std::get<2>(_header).starts_with(LZ4)),
  _compressedSize(std::get<1>(_header)),
  _uncompressedSize(std::get<0>(_header)),
  _diagnostics(std::get<3>(_header)) {}

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

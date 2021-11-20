#include "Utility.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <cassert>

namespace utility {

std::string createIndexPrefix(size_t index) {
  static const std::string error = "Error";
  std::string indexStr;
  char arr[CONV_BUFFER_SIZE] = {};
  if (auto [ptr, ec] = std::to_chars(arr, arr + CONV_BUFFER_SIZE, index);
      ec == std::errc())
    return indexStr.append(1, '[').append(arr, ptr - arr).append(1, ']');
  else {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__
	      << "-error translating number" << std::endl;
    return error;
  }
}

bool extractLines(std::string_view input,
		  std::vector<std::string>& lines,
		  bool lastDelim,
		  char delim) {
  if (input.empty())
    return lastDelim;
  std::vector<std::string_view> partialLines;
  split(input, partialLines, delim);
  if (lastDelim)
    lines.insert(lines.end(), partialLines.begin(), partialLines.end());
  else {
    lines.back().append(partialLines[0]);
    if (partialLines.size() > 1)
      lines.insert(lines.end(), ++partialLines.begin(), partialLines.end());
  }
  return input.back() == delim;
}

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, std::string_view compressor) {
  bool ok = toChars(uncomprSz, buffer, NUM_FIELD_SIZE);
  assert(ok);
  ok = toChars(comprSz, buffer + NUM_FIELD_SIZE, NUM_FIELD_SIZE);
  assert(ok);
  std::strncpy(buffer + NUM_FIELD_SIZE * 2, compressor.data(), COMPRESSOR_NAME_SIZE - 1);
}

HEADER decodeHeader(std::string_view buffer, bool done) {
  size_t uncomprSize = 0;
  std::string_view stru(buffer.data(), NUM_FIELD_SIZE);
  if (!fromChars(stru, uncomprSize)) {
    return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false);
  }
  size_t comprSize = 0;
  std::string_view strc(buffer.data() + NUM_FIELD_SIZE, NUM_FIELD_SIZE);
  if (!fromChars(strc, comprSize)) {
    return std::make_tuple(-1, -1, EMPTY_COMPRESSOR, false);
  }
  std::string_view compressor(buffer.data() + NUM_FIELD_SIZE * 2, COMPRESSOR_NAME_SIZE);
  bool enabled = compressor.starts_with(LZ4);
  return std::make_tuple(uncomprSize, comprSize, enabled ? LZ4 : EMPTY_COMPRESSOR, done);
}

// reduce number of write/read system calls.
bool mergePayload(const Batch& batch, Batch& aggregatedBatch) {
  if (batch.empty())
    return false;
  static const size_t BUFFER_SIZE = ProgramOptions::get("DYNAMIC_BUFFER_SIZE", 200000);
  std::string bigString;
  size_t reserveSize = 0;
  if (reserveSize)
    bigString.reserve(reserveSize + 1);
  for (std::string_view line : batch) {
    if (bigString.size() + line.size() < BUFFER_SIZE || bigString.empty())
      bigString.append(line);
    else {
      reserveSize = std::max(reserveSize, bigString.size());
      aggregatedBatch.push_back(std::move(bigString));
      bigString.clear();
      bigString.assign(line);
    }
  }
  if (!bigString.empty())
    aggregatedBatch.push_back(std::move(bigString));
  return true;
}

bool buildMessage(const Batch& payload, Batch& message) {
  if (payload.empty())
    return false;
  for (std::string_view str : payload) {
    static const bool testCompression = ProgramOptions::get("TestCompression", false);
    if (testCompression)
      assert(Compression::testCompressionDecompression(str));
    char array[HEADER_SIZE] = {};
    static const auto[compressor, enabled] = Compression::isCompressionEnabled();
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string compressed; // may be unused if pooled buffer is big enough
      std::string_view dstView = Compression::compress(str, compressed);
      if (dstView.empty())
	return false;
      encodeHeader(array, uncomprSize, dstView.size(), compressor);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      encodeHeader(array, uncomprSize, uncomprSize, compressor);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}

} // end of namespace utility

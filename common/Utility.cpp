/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include "Compression.h"
#include "ProgramOptions.h"
#include <cassert>
#include <cstring>

namespace utility {

void encodeHeader(char* buffer, size_t uncomprSz, size_t comprSz, std::string_view compressor) {
  std::memset(buffer, 0, HEADER_SIZE);
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

std::string createRequestId(size_t index) {
  char arr[CONV_BUFFER_SIZE] = { '[' };
  auto [ptr, ec] = std::to_chars(arr + 1, arr + CONV_BUFFER_SIZE, index);
  assert(ec == std::errc());
  *ptr = ']';
  return arr;
}

bool preparePackage(const Batch& payload, Batch& modified) {
  modified.clear();
  // keep vector capacity
  static Batch aggregated;
  aggregated.clear();
  if (!mergePayload(payload, aggregated)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  if (!(buildMessage(aggregated, modified))) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  return true;
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
    char array[HEADER_SIZE] = {};
    static const auto[compressor, enabled] = Compression::isCompressionEnabled();
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string_view dstView = Compression::compress(str);
      if (dstView.empty())
	return false;
      utility::encodeHeader(array, uncomprSize, dstView.size(), compressor);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      utility::encodeHeader(array, uncomprSize, uncomprSize, compressor);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}

} // end of namespace utility

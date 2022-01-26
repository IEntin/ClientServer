/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include "Compression.h"
#include "Header.h"
#include "ProgramOptions.h"
#include <cassert>
#include <cstring>

namespace utility {

std::string createRequestId(size_t index) {
  char arr[CONV_BUFFER_SIZE + 1] = { '[' };
  auto [ptr, ec] = std::to_chars(arr + 1, arr + CONV_BUFFER_SIZE, index);
  assert(ec == std::errc() && ptr - arr < CONV_BUFFER_SIZE);
  *ptr = ']';
  return arr;
}

bool preparePackage(const Batch& payload, Batch& modified) {
  static const bool diagnostics = ProgramOptions::get("Diagnostics", false);
  modified.clear();
  // keep vector capacity
  static Batch aggregated;
  aggregated.clear();
  if (!mergePayload(payload, aggregated)) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ":failed" << std::endl;
    return false;
  }
  if (!(buildMessage(aggregated, modified, diagnostics))) {
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
      bigString.assign(std::move(line));
    }
  }
  if (!bigString.empty())
    aggregatedBatch.push_back(std::move(bigString));
  return true;
}

bool buildMessage(const Batch& payload, Batch& message, bool diagnostics) {
  if (payload.empty())
    return false;
  for (std::string_view str : payload) {
    char array[HEADER_SIZE + 1] = {};
    static const auto[compressor, enabled] = Compression::isCompressionEnabled();
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string_view dstView = Compression::compress(str);
      if (dstView.empty())
	return false;
      encodeHeader(array, uncomprSize, dstView.size(), compressor, diagnostics);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      encodeHeader(array, uncomprSize, uncomprSize, compressor, diagnostics);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}

} // end of namespace utility

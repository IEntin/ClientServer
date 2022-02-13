#include "Client.h"
#include "Compression.h"
#include "Header.h"
#include "ClientOptions.h"
#include "Utility.h"
#include <cassert>
#include <cstring>
#include <sstream>

Client::Client(const ClientOptions& options) : _options(options) {}

bool Client::preparePackage(const Batch& payload, Batch& modified, size_t bufferSize) {
  modified.clear();
  // keep vector capacity
  static Batch aggregated;
  aggregated.clear();
  if (!mergePayload(payload, aggregated, bufferSize)) {
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
bool Client::mergePayload(const Batch& batch, Batch& aggregatedBatch, size_t bufferSize) {
  if (batch.empty())
    return false;
  std::string bigString;
  size_t reserveSize = 0;
  if (reserveSize)
    bigString.reserve(reserveSize + 1);
  for (std::string_view line : batch) {
    if (bigString.size() + line.size() < bufferSize || bigString.empty())
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

bool Client::buildMessage(const Batch& payload, Batch& message) {
  if (payload.empty())
    return false;
  const auto[compressor, enabled] = _options._compression;
  static auto& printOnce[[maybe_unused]] =
    std::clog << LZ4 << "compression " << (enabled ? "enabled" : "disabled") << std::endl;
  for (std::string_view str : payload) {
    char array[HEADER_SIZE + 1] = {};
    size_t uncomprSize = str.size();
    message.emplace_back();
    if (enabled) {
      std::string_view dstView = Compression::compress(str);
      if (dstView.empty())
	return false;
      encodeHeader(array, uncomprSize, dstView.size(), compressor, _options._diagnostics);
      message.back().reserve(HEADER_SIZE + dstView.size() + 1);
      message.back().append(array, HEADER_SIZE).append(dstView);
    }
    else {
      encodeHeader(array, uncomprSize, uncomprSize, compressor, _options._diagnostics);
      message.back().reserve(HEADER_SIZE + str.size() + 1);
      message.back().append(array, HEADER_SIZE).append(str);
    }
  }
  return true;
}

std::string Client::createRequestId(size_t index) {
  char arr[CONV_BUFFER_SIZE + 1] = { '[' };
  auto [ptr, ec] = std::to_chars(arr + 1, arr + CONV_BUFFER_SIZE, index);
  assert(ec == std::errc() && ptr - arr < CONV_BUFFER_SIZE);
  *ptr = ']';
  return arr;
}

size_t Client::createPayload(const char* sourceName, Batch& payload) {
  std::ifstream input(sourceName, std::ifstream::in | std::ifstream::binary);
  if (!input)
    throw std::runtime_error(sourceName);
  unsigned long long requestIndex = 0;
  std::string line;
  Batch batch;
  while (std::getline(input, line)) {
    if (line.empty())
      continue;
    std::string modifiedLine(createRequestId(requestIndex++));
    modifiedLine.append(line.append(1, '\n'));
    payload.emplace_back(std::move(modifiedLine));
  }
  return payload.size();
}

std::string Client::readFileContent(const std::string& name) {
  std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << name << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

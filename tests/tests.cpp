/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommUtility.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "Utility.h"
#include <gtest/gtest.h>
#include <cstring>
#include <fstream>
#include <iostream>

constexpr const char* sourceName = "client_bin/requests.log";

size_t getMemPoolBufferSize() {
  return 100000;
}

std::string readContent(const std::string& name = sourceName) {
  std::ifstream ifs(sourceName, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << sourceName << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

bool testCompressionDecompression1(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  if (compressedView.empty())
    return false;
  // save in a string before buffer is reused in uncompress
  std::string compressed;
  compressed.assign(compressedView.data(), compressedView.size());
  std::string_view uncompressedView = Compression::uncompress(compressed, input.size());
  static auto& printOnce [[maybe_unused]] = std::clog << "\n   input.size()=" << input.size()
	    << " compressedView.size()=" << compressedView.size() << " restored to original:"
            << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
  return input == uncompressedView;
}

bool testCompressionDecompression2(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  if (compressedView.empty())
    return false;
  std::vector<char> uncompressed(input.size());
  if (!Compression::uncompress(compressedView, uncompressed))
    return false;
  std::string_view uncompressedView(uncompressed.begin(), uncompressed.end());
  static auto& printOnce [[maybe_unused]] = std::clog << "\n   input.size()=" << input.size()
            << " compressedView.size()=" << compressedView.size() << " restored to original:"
	    << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
  return input == uncompressedView;
}

TEST(CompressionTest, CompressionTest1) {
  bool compressionTestResult = true;
  const std::string content = readContent();
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression1(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(CompressionTest, CompressionTest2) {
  bool compressionTestResult = true;
  const std::string content = readContent("client_bin/output.txt");
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression2(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(SplitTest, SplitTest1) {
  const std::string content = readContent("client_bin/requests.log");
  std::vector<std::string_view> lines;
  utility::split(content, lines);
  ASSERT_EQ(lines.size(), 10000);
}

TEST(FromCharsTest, Integral) {
  const std::string_view view = "123456789";
  int value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 123456789);
}

TEST(FromCharsTest, FloatingPoint) {
  const std::string_view view = "1234567.89";
  double value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 1234567.89);
}

TEST(HeaderTest, HeaderTest1) {
  char buffer[HEADER_SIZE] = {};
  size_t uncomprSz = 123456;
  size_t comprSz = 12345;
  COMPRESSORS compressor = COMPRESSORS::LZ4;
  bool diagnostics = true;
  encodeHeader(buffer, uncomprSz, comprSz, compressor, diagnostics);
  HEADER header = decodeHeader(std::string_view(buffer, HEADER_SIZE), true);
  ASSERT_TRUE(isOk(header));
  size_t uncomprSzResult = getUncompressedSize(header);
  ASSERT_EQ(uncomprSz, uncomprSzResult);
  size_t comprSzResult = getCompressedSize(header);
  ASSERT_EQ(comprSz, comprSzResult);
  COMPRESSORS compressorResult = getCompressor(header);
  ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
  bool diagnosticsResult = getDiagnostics(header);
  ASSERT_EQ(diagnostics, diagnosticsResult);
  compressor = COMPRESSORS::NONE;
  encodeHeader(buffer, uncomprSz, comprSz, compressor, diagnostics);
  header = decodeHeader(std::string_view(buffer, HEADER_SIZE), true);
  ASSERT_TRUE(isOk(header));
  compressorResult = getCompressor(header);
  ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
}

TEST(PreparePackageTest, PreparePackageTest1) {
  Batch payload;
  commutility::createPayload(sourceName, payload);
  Batch modified;
  size_t bufferSize = 360000;
  bool diagnostics = true;
  bool prepared = utility::preparePackage(payload, modified, bufferSize, diagnostics);
  ASSERT_TRUE(prepared);
  ASSERT_TRUE(Compression::isCompressionEnabled().second);
}

int main(int argc, char **argv) {
  MemoryPool::setup(200000);
  Compression::setCompressionEnabled("LZ4");
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

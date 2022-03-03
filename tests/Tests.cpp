/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Client.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "TaskBuilder.h"
#include "Utility.h"
#include <gtest/gtest.h>
#include <csignal>
#include <fstream>
#include <iostream>
#include <sstream>

volatile std::sig_atomic_t stopFlag = false;

struct CompressionTest : testing::Test {
  static std::string _input1;
  static std::string _input2;
};
std::string CompressionTest::_input1 = Client::readFile("requests.log");
std::string CompressionTest::_input2 = Client::readFile("output.txt");

void testCompressionDecompression1(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  ASSERT_FALSE(compressedView.empty());
  // save to a string before buffer is reused in uncompress
  std::string compressed;
  compressed.assign(compressedView.data(), compressedView.size());
  std::string_view uncompressedView = Compression::uncompress(compressed, input.size());
  static auto& printOnce [[maybe_unused]] = std::clog << "\n   input.size()=" << input.size()
	    << " compressedView.size()=" << compressedView.size() << " restored to original:"
            << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
  ASSERT_EQ(input, uncompressedView);
}

void testCompressionDecompression2(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  ASSERT_FALSE(compressedView.empty());
  std::vector<char> uncompressed(input.size());
  ASSERT_TRUE(Compression::uncompress(compressedView, uncompressed));
  std::string_view uncompressedView(uncompressed.begin(), uncompressed.end());
  static auto& printOnce [[maybe_unused]] = std::clog << "\n   input.size()=" << input.size()
            << " compressedView.size()=" << compressedView.size() << " restored to original:"
	    << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
  ASSERT_EQ(input, uncompressedView);
}

TEST_F(CompressionTest, CompressionTest1) {
  for (int i = 0; i < 5; ++i)
    testCompressionDecompression1(_input1);
}

TEST_F(CompressionTest, CompressionTest2) {
  for (int i = 0; i < 5; ++i)
    testCompressionDecompression1(_input2);
}

TEST_F(CompressionTest, CompressionTest3) {
  for (int i = 0; i < 5; ++i)
    testCompressionDecompression2(_input1);
}

TEST_F(CompressionTest, CompressionTest4) {
  for (int i = 0; i < 5; ++i)
    testCompressionDecompression2(_input2);
}

TEST(SplitTest, SplitTest1) {
  const std::string content = Client::readFile("requests.log");
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
  bool diagnosticsResult = isDiagnosticsEnabled(header);
  ASSERT_EQ(diagnostics, diagnosticsResult);
  compressor = COMPRESSORS::NONE;
  encodeHeader(buffer, uncomprSz, comprSz, compressor, diagnostics);
  header = decodeHeader(std::string_view(buffer, HEADER_SIZE), true);
  ASSERT_TRUE(isOk(header));
  compressorResult = getCompressor(header);
  ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
}

struct BuildTaskTest : testing::Test {

  void testBuildTask(COMPRESSORS compressor) {
    ClientOptions options;
    options._compressor = compressor;
    TaskBuilder taskBuilder(options._sourceName, options._compressor, options._diagnostics);
    Batch originalBatch;
    taskBuilder.createRequestBatch(originalBatch);
    taskBuilder.run();
    Batch task;
    taskBuilder.getTask(task);
    ASSERT_TRUE(taskBuilder.isDone());
    std::string uncompressedResult;
    for (const std::string& subtask : task) {
      HEADER header = decodeHeader(subtask.data());
      ASSERT_TRUE(isOk(header));
      bool bcompressed = isInputCompressed(header);
      ASSERT_EQ(bcompressed, compressor == COMPRESSORS::LZ4);
      size_t comprSize = getCompressedSize(header);
      ASSERT_EQ(comprSize + HEADER_SIZE, subtask.size());
      if (bcompressed) {
	std::string_view uncompressedView =
	  Compression::uncompress(std::string_view(subtask.data() + HEADER_SIZE, subtask.size() - HEADER_SIZE),
				  getUncompressedSize(header));
	ASSERT_FALSE(uncompressedView.empty());
	uncompressedResult.append(uncompressedView);
      }
      else
	uncompressedResult.append(subtask.data() + HEADER_SIZE, subtask.size() - HEADER_SIZE);
    }
    Batch batchResult;
    utility::split(uncompressedResult, batchResult);
    Batch batchOfSingles;
    for (const std::string& bigString : originalBatch) {
      Batch subBatch;
      utility::split(bigString, subBatch);
      batchOfSingles.insert(batchOfSingles.end(),
			    std::make_move_iterator(subBatch.begin()),
			    std::make_move_iterator(subBatch.end()));
    }
    ASSERT_TRUE(batchResult.size() == batchOfSingles.size());
    ASSERT_TRUE(batchResult == batchOfSingles);
  }

  static void SetUpTestSuite() {}

  static void TearDownTestSuite() {}
};

TEST_F(BuildTaskTest, Compression) {
  testBuildTask(COMPRESSORS::LZ4);
}

TEST_F(BuildTaskTest, NoCompression) {
  testBuildTask(COMPRESSORS::NONE);
}

int main(int argc, char **argv) {
  MemoryPool::setup(200000);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

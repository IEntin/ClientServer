/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "TaskBuilder.h"
#include "TestEnvironment.h"
#include "Utility.h"
#include <gtest/gtest.h>

struct CompressionTest : testing::Test {

  void testCompressionDecompression1(std::string_view input) {
    MemoryPool memoryPool;
    memoryPool.setInitialSize(100000);
    std::string_view compressedView = Compression::compress(input, memoryPool);
    ASSERT_FALSE(compressedView.empty());
    // save to a string before buffer is reused in uncompress
    std::string compressed;
    compressed.assign(compressedView.data(), compressedView.size());
    std::string_view uncompressedView = Compression::uncompress(compressed, input.size(), memoryPool);
    static auto& printOnce [[maybe_unused]] =
      std::clog << "\n   input.size()=" << input.size()
		<< " compressedView.size()=" << compressedView.size() << " restored to original:"
		<< std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
    ASSERT_EQ(input, uncompressedView);
  }

  void testCompressionDecompression2(std::string_view input) {
    MemoryPool memoryPool;
    memoryPool.setInitialSize(100000);
    std::string_view compressedView = Compression::compress(input, memoryPool);
    ASSERT_FALSE(compressedView.empty());
    std::vector<char> uncompressed(input.size());
    ASSERT_TRUE(Compression::uncompress(compressedView, uncompressed));
    std::string_view uncompressedView(uncompressed.begin(), uncompressed.end());
    static auto& printOnce [[maybe_unused]] =
      std::clog << "\n   input.size()=" << input.size()
		<< " compressedView.size()=" << compressedView.size() << " restored to original:"
		<< std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
    ASSERT_EQ(input, uncompressedView);
  }

  static void SetUpTestSuite() {}
  static void TearDownTestSuite() {}
};

TEST_F(CompressionTest, 1_SOURCE) {
  testCompressionDecompression1(TestEnvironment::_source);
}

TEST_F(CompressionTest, 1_OUTPUTD) {
  testCompressionDecompression1(TestEnvironment::_outputD);
}

TEST_F(CompressionTest, 2_SOURCE) {
  testCompressionDecompression2(TestEnvironment::_source);
}

TEST_F(CompressionTest, 2_OUTPUTD) {
  testCompressionDecompression2(TestEnvironment::_outputD);
}

TEST(SplitTest, NoKeepDelim) {
  ClientOptions clientOptions;
  std::vector<std::string_view> lines;
  utility::split(TestEnvironment::_source, lines);
  ASSERT_EQ(lines.size(), 10000);
  for (const auto& line : lines)
    ASSERT_FALSE(line.ends_with('\n'));
}

TEST(SplitTest, KeepDelim) {
  ClientOptions clientOptions;
  std::vector<std::string_view> lines;
  utility::split(TestEnvironment::_source, lines, '\n', 1);
  ASSERT_EQ(lines.size(), 10000);
  for (const auto& line : lines)
    ASSERT_TRUE(line.ends_with('\n'));
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

TEST(HeaderTest, 1) {
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
    MemoryPool memoryPool;
    memoryPool.setInitialSize(options._bufferSize);
    TaskBuilder taskBuilder(options, memoryPool);
    taskBuilder.run();
    Vectors task;
    ASSERT_TRUE(taskBuilder.getTask(task));
    std::string uncompressedResult;
    for (const auto& subtask : task) {
      HEADER header = decodeHeader(subtask.data());
      ASSERT_TRUE(isOk(header));
      bool bcompressed = isInputCompressed(header);
      ASSERT_EQ(bcompressed, compressor == COMPRESSORS::LZ4);
      size_t comprSize = getCompressedSize(header);
      ASSERT_EQ(comprSize + HEADER_SIZE, subtask.size());
      if (bcompressed) {
	std::string_view uncompressedView =
	  Compression::uncompress(std::string_view(subtask.data() + HEADER_SIZE, subtask.size() - HEADER_SIZE),
				  getUncompressedSize(header), memoryPool);
	ASSERT_FALSE(uncompressedView.empty());
	uncompressedResult.append(uncompressedView);
      }
      else
	uncompressedResult.append(subtask.data() + HEADER_SIZE, subtask.size() - HEADER_SIZE);
    }
    std::vector<std::string_view> batchResult;
    utility::split(uncompressedResult, batchResult);
    std::string restoredString;
    for (const auto& item : batchResult) {
      std::string_view str(item.data(), item.size());
      size_t pos = str.find(']');
      restoredString.append(str.substr(pos + 1)).append(1, '\n');
    }
    ASSERT_EQ(TestEnvironment::_source.size(), restoredString.size());
    ASSERT_EQ(TestEnvironment::_source, restoredString);
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

int main(int argc, char** argv) {
  TestEnvironment* env = new TestEnvironment();
  ::testing::AddGlobalTestEnvironment(env);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

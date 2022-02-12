/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "CommUtility.h"
#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "MemoryPool.h"
#include "ThreadPool.h"
#include "Utility.h"
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>
#include <sstream>

bool testCompressionDecompression1(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  if (compressedView.empty())
    return false;
  // save to a string before buffer is reused in uncompress
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
  const std::string content = commutility::readFileContent("requests.log");
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression1(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(CompressionTest, CompressionTest2) {
  bool compressionTestResult = true;
  const std::string content = commutility::readFileContent("output.txt");
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression2(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(SplitTest, SplitTest1) {
  const std::string content = commutility::readFileContent("requests.log");
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

TEST(PreparePackageTest, PreparePackageTestCompressed) {
  Compression::setCompressionEnabled(std::string(LZ4));
  Batch payload;
  commutility::createPayload("requests.log", payload);
  Batch modified;
  size_t bufferSize = 360000;
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::LZ4, true);
  ClientOptions options(compression);
  bool prepared = utility::preparePackage(payload, modified, bufferSize, options);
  ASSERT_TRUE(prepared);
  std::string uncompressedResult;
  for (const std::string& task : modified) {
    HEADER header = decodeHeader(task.data());
    ASSERT_TRUE(isOk(header));
    bool bcompressed = isInputCompressed(header);
    ASSERT_TRUE(bcompressed);
    size_t comprSize = getCompressedSize(header);
    ASSERT_EQ(comprSize + HEADER_SIZE, task.size());
    if (bcompressed) {
      std::string_view uncompressedView =
	Compression::uncompress(std::string_view(task.data() + HEADER_SIZE, task.size() - HEADER_SIZE),
				getUncompressedSize(header));
      ASSERT_FALSE(uncompressedView.empty());
      uncompressedResult.append(uncompressedView);
    }
    else
      uncompressedResult.append(task.data() + HEADER_SIZE, task.size() - HEADER_SIZE);
  }
  Batch batchResult;
  utility::split(uncompressedResult, batchResult);
  for (auto& line : batchResult)
    line.append(1, '\n');
  ASSERT_TRUE(batchResult == payload);
}

TEST(PreparePackageTest, PreparePackageTestNotcompressed) {
  Compression::setCompressionEnabled(std::string(NOP));
  Batch payload;
  commutility::createPayload("requests.log", payload);
  Batch modified;
  size_t bufferSize = 360000;
  auto compression = std::make_pair<COMPRESSORS, bool>(COMPRESSORS::NONE, false);
  ClientOptions options(compression);
  bool prepared = utility::preparePackage(payload, modified, bufferSize, options);
  ASSERT_TRUE(prepared);
  std::string uncompressedResult;
  for (const std::string& task : modified) {
    HEADER header = decodeHeader(task.data());
    ASSERT_TRUE(isOk(header));
    bool bcompressed = isInputCompressed(header);
    ASSERT_FALSE(bcompressed);
    size_t comprSize = getCompressedSize(header);
    ASSERT_EQ(comprSize + HEADER_SIZE, task.size());
    if (bcompressed) {
      std::string_view uncompressedView =
	Compression::uncompress(std::string_view(task.data() + HEADER_SIZE, task.size() - HEADER_SIZE),
				getUncompressedSize(header));
      ASSERT_FALSE(uncompressedView.empty());
      uncompressedResult.append(uncompressedView);
    }
    else
      uncompressedResult.append(task.data() + HEADER_SIZE, task.size() - HEADER_SIZE);
  }
  Batch batchResult;
  utility::split(uncompressedResult, batchResult);
  for (auto& line : batchResult)
    line.append(1, '\n');
  ASSERT_TRUE(batchResult == payload);
}

TEST(ThreadPoolTest, ThreadPoolTest1) {
  ThreadPool pool(10);
  class TestRunnable : public std::enable_shared_from_this<TestRunnable>, public Runnable {
  public:
    TestRunnable(unsigned number, ThreadPool& pool) : Runnable(), _number(number), _pool(pool) {}
    ~TestRunnable() override {
      EXPECT_EQ(_id, std::this_thread::get_id());
    }
    void run() override {
      _id = std::this_thread::get_id();
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    void start() {
      _pool.get().push(shared_from_this());
    }
    const unsigned _number;
    std::reference_wrapper<ThreadPool> _pool;
    std::thread::id _id;
  };
  for (unsigned i = 0; i < 20; ++i) {
    auto runnable = std::make_shared<TestRunnable>(i, pool);
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    runnable->start();
  }
  pool.stop();
  bool allJoined = true;
  for (auto& thread : pool.getThreads())
    allJoined = allJoined && !thread.joinable();
  ASSERT_TRUE(allJoined);
}

int main(int argc, char **argv) {
  MemoryPool::setup(200000);
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

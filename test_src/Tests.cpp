/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Compression.h"
#include "Header.h"
#include "Logger.h"
#include "TestEnvironment.h"
#include "Utility.h"

struct CompressionTest : testing::Test {

  void testCompressionDecompression1(std::string_view input) {
    try{
      std::string_view compressedView = Compression::compress(input.data(), input.size());
      // save to a string before buffer is reused in uncompress
      std::string compressed(compressedView.data(), compressedView.size());
      std::string_view uncompressedView = Compression::uncompress(compressed.data(), compressed.size(), input.size());
      // ERROR level to make this log visible in gtest
      static auto& printOnce [[maybe_unused]] =
	Logger(LOG_LEVEL::ERROR, std::cerr) << "\n   input.size()=" << input.size()
	     << " compressedView.size()=" << compressedView.size() << " restored to original:"
	     << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
      ASSERT_EQ(input, uncompressedView);
    }
    catch (const std::exception& e) {
      Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
    }
  }

  void testCompressionDecompression2(std::string_view input) {
    try{
      std::string_view compressedView = Compression::compress(input.data(), input.size());
      // supplied external buffers to uncompress
      std::vector<char> uncompressed(input.size());
      std::vector<char> compressed(compressedView.cbegin(), compressedView.cend());
      ASSERT_TRUE(Compression::uncompress(compressed, compressed.size(), uncompressed));
      ASSERT_EQ(input, std::string_view(uncompressed.data(), uncompressed.size()));
    }
    catch (const std::exception& e) {
      Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
    }
  }
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
  std::vector<std::string_view> lines;
  utility::split(TestEnvironment::_source, lines);
  ASSERT_EQ(lines.size(), 10000);
  for (const auto& line : lines)
    ASSERT_FALSE(line.ends_with('\n'));
}

TEST(SplitTest, KeepDelim) {
  std::vector<std::string_view> lines;
  utility::split(TestEnvironment::_source, lines, '\n', 1);
  ASSERT_EQ(lines.size(), 10000);
  for (const auto& line : lines)
    ASSERT_TRUE(line.ends_with('\n'));
}

TEST(SplitTest, Chars) {
  std::vector<std::string_view> chars;
  std::string source("A|B|C|D|E");
  utility::split(source, chars, '|', 1);
  std::erase(source, '|');
  ASSERT_EQ(chars.size(), source.size());
}

TEST(SplitTest, MultiDelims) {
  std::vector<std::string_view> chars;
  std::string source("A|*B|*C|D|*E");
  utility::split(source, chars, "|*");
  std::erase(source, '|');
  std::erase(source, '*');
  ASSERT_EQ(chars.size(), source.size());
}

TEST(ToCharsTest, Integral) {
  int value = 7;
  int shift = 2;
  constexpr int size = 7;
  char array[size] = {};
  utility::toChars(value, array + shift, sizeof(array));
  ASSERT_TRUE(*(array + shift) == '0' + value);
}

TEST(FromCharsTest, Integral0) {
  std::string_view view = "0000";
  int value = 7;
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 0);
}

TEST(FromCharsTest, Integral) {
  std::string_view view = "123456789";
  int value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 123456789);
}

TEST(FromCharsTest, FloatingPoint) {
  std::string_view view = "1234567.89";
  double value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 1234567.89);
}

TEST(HeaderTest, 1) {
  try {
    char buffer[HEADER_SIZE] = {};
    size_t uncomprSz = 123456;
    size_t comprSz = 12345;
    COMPRESSORS compressor = COMPRESSORS::LZ4;
    bool diagnostics = true;
    encodeHeader(buffer, HEADERTYPE::SESSION, uncomprSz, comprSz, compressor, diagnostics);
    HEADER header = decodeHeader(buffer);
    size_t uncomprSzResult = extractUncompressedSize(header);
    ASSERT_EQ(uncomprSz, uncomprSzResult);
    size_t comprSzResult = extractCompressedSize(header);
    ASSERT_EQ(comprSz, comprSzResult);
    COMPRESSORS compressorResult = extractCompressor(header);
    ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
    bool diagnosticsResult = isDiagnosticsEnabled(header);
    ASSERT_EQ(diagnostics, diagnosticsResult);
    compressor = COMPRESSORS::NONE;
    encodeHeader(buffer, HEADERTYPE::SESSION, uncomprSz, comprSz, compressor, diagnostics);
    header = decodeHeader(buffer);
    compressorResult = extractCompressor(header);
    ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
  }
  catch (const std::exception& e) {
    Logger(LOG_LEVEL::ERROR, std::cerr) << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':' << e.what() << std::endl;
  }
}

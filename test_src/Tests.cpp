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
      static thread_local std::vector<char> buffer;
      buffer.clear();
      std::string_view compressedView = Compression::compress(input, buffer);
      std::vector<char> uncompressed(input.size());
      Compression::uncompress(compressedView, uncompressed);
      std::string_view uncompressedView(uncompressed.data(), uncompressed.size());
      // ERROR level to make this log visible in gtest
      static auto& printOnce [[maybe_unused]] =
	Logger(false) << "\n   input.size()=" << input.size()
		      << " compressedView.size()=" << compressedView.size() << " restored to original:"
		      << std::boolalpha << (input == uncompressedView) << '\n' << std::endl;
      ASSERT_EQ(input, uncompressedView);
    }
    catch (const std::exception& e) {
      LogError << e.what() << std::endl;
    }
  }
};

TEST_F(CompressionTest, 1_SOURCE) {
  testCompressionDecompression1(TestEnvironment::_source);
}

TEST_F(CompressionTest, 1_OUTPUTD) {
  testCompressionDecompression1(TestEnvironment::_outputD);
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
    size_t payloadSz = 123567;
    size_t uncomprSz = 123456;
   COMPRESSORS compressor = COMPRESSORS::LZ4;
    bool encrypted = false;
    bool diagnostics = true;
    encodeHeader(buffer, HEADERTYPE::SESSION, payloadSz, uncomprSz, compressor, encrypted, diagnostics);
    HEADER header = decodeHeader(buffer);
    size_t payloadSzResult = extractPayloadSize(header);
    ASSERT_EQ(payloadSz, payloadSzResult);
    size_t uncomprSzResult = extractUncompressedSize(header);
    ASSERT_EQ(uncomprSz, uncomprSzResult);
    COMPRESSORS compressorResult = extractCompressor(header);
    ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
    bool encryptedResult = isEncrypted(header);
    ASSERT_EQ(encrypted, encryptedResult);
    bool diagnosticsResult = isDiagnosticsEnabled(header);
    ASSERT_EQ(diagnostics, diagnosticsResult);
    compressor = COMPRESSORS::NONE;
    encodeHeader(buffer, HEADERTYPE::SESSION, payloadSz, uncomprSz, compressor, encrypted, diagnostics);
    header = decodeHeader(buffer);
    compressorResult = extractCompressor(header);
    ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
  }
  catch (const std::exception& e) {
    LogError << e.what() << std::endl;
    ASSERT_TRUE(false);
  }
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "TestEnvironment.h"
#include "Utility.h"

struct CompressionTest : testing::Test {

  void testCompressionDecompression(std::string& input) {
    try{
      std::string data = input;
      std::string_view compressed = compression::compress(input);
      size_t compressedSize = compressed.size();
      std::string_view uncompressed = compression::uncompress(compressed, input.size());
      static auto& printOnce [[maybe_unused]] =
	Logger(LOG_LEVEL::ALWAYS, std::clog, false)
	<< "\n\tinput.size()=" << input.size()
	<< " compressedSize=" << compressedSize << " restored to original:"
	<< std::boolalpha << (input == uncompressed) << '\n' << '\n';
      ASSERT_EQ(input, uncompressed);
    }
    catch (const std::exception& e) {
      LogError << e.what() << '\n';
    }
  }
};

TEST_F(CompressionTest, 1_SOURCE) {
  testCompressionDecompression(TestEnvironment::_source);
}

TEST_F(CompressionTest, 1_OUTPUTD) {
  testCompressionDecompression(TestEnvironment::_outputD);
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
  std::string contents;
  utility::readFile(ClientOptions::_sourceName, contents);
  utility::split(contents, lines, '\n', 1);
  ASSERT_EQ(lines.size(), 10000);
  ASSERT_TRUE(contents.starts_with(lines[0]));
  ASSERT_TRUE(contents.ends_with(lines[9999]));
  ASSERT_FALSE(contents.find(lines[5432]) == std::string::npos);
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
    unsigned payloadSz = 123567;
    unsigned uncomprSz = 123456;
    COMPRESSORS compressor = COMPRESSORS::LZ4;
    bool encrypted = false;
    bool diagnostics = true;
    HEADER header{HEADERTYPE::SESSION, payloadSz, uncomprSz, compressor, encrypted, diagnostics, STATUS::NONE};
    encodeHeader(buffer, header);
    header = decodeHeader(buffer);
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
    header = {HEADERTYPE::SESSION, payloadSz, uncomprSz, compressor, encrypted, diagnostics, STATUS::NONE};
    encodeHeader(buffer, header);
    header = decodeHeader(buffer);
    compressorResult = extractCompressor(header);
    ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
  }
  catch (const std::exception& e) {
    LogError << e.what() << '\n';
    ASSERT_TRUE(false);
  }
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Compression.h"
#include "Header.h"
#include "IoUtility.h"
#include "Lines.h"
#include "TestEnvironment.h"
#include "Utility.h"

// ./testbin --gtest_filter=GetLineTest*
// gdb --args testbin --gtest_filter=GetLineTest*

struct CompressionTest : testing::Test {
  void testCompressionDecompression(std::string& input) {
    std::string_view compressed = compression::compress(input);
    std::string_view uncompressed = compression::uncompress(compressed, input.size());
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    static auto& printOnce [[maybe_unused]] = logger
      << "\n\tinput.size()=" << input.size()
      << " compressedSize=" << compressed.size() << " restored to original:"
      << std::boolalpha << (input == uncompressed) << '\n' << '\n';
    ASSERT_EQ(input, uncompressed);
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
  utility::splitFast(TestEnvironment::_source, lines);
  ASSERT_EQ(lines.size(), 10000);
  for (const auto& line : lines)
    ASSERT_FALSE(line.ends_with('\n'));
}

TEST(SplitTest, KeepDelim) {
  std::vector<std::string_view> lines;
  std::string contents;
  utility::readFile(ClientOptions::_sourceName, contents);
  utility::splitFast(contents, lines, '\n', 1);
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
  ioutility::toChars(value, array + shift);
  ASSERT_TRUE(array[shift] == '0' + value);
}

TEST(FromCharsTest, Integral0) {
  std::string_view view = "0000";
  int value = 7;
  ioutility::fromChars(view, value);
  ASSERT_EQ(value, 0);
}

TEST(FromCharsTest, Integral) {
  std::string_view view = "123456789";
  int value = {};
  ioutility::fromChars(view, value);
  ASSERT_EQ(value, 123456789);
}

TEST(FromCharsTest, FloatingPoint) {
  std::string_view view = "1234567.89";
  double value = {};
  ioutility::fromChars(view, value);
  ASSERT_EQ(value, 1234567.89);
}

TEST(HeaderTest, 1) {
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

TEST(GetLineTest, 1) {
  // get last line in the file using std::getline
  std::string lastLine;
  ASSERT_TRUE(utility::getLastLine(ClientOptions::_sourceName, lastLine));
  if (utility::fileEndsWithEOL(ClientOptions::_sourceName))
    lastLine.push_back('\n');

  // use Lines::getLine
  std::string_view lineView;
  Lines linesV(ClientOptions::_sourceName, '\n', true);
  while (linesV.getLine(lineView)) {
    if (linesV._last) {
      ASSERT_EQ(lineView, lastLine);
    }
  }

  std::string lineStr;
  Lines linesS(ClientOptions::_sourceName, '\n', true);
  while (linesS.getLine(lineStr)) {
    if (linesS._last) {
      ASSERT_EQ(lineStr, lastLine);
    }
  }
  // discard delimiter:
  // keepDelimiter == false by default
  Lines linesDiscardDelimiter(ClientOptions::_sourceName);
  lastLine.pop_back();
  while (linesDiscardDelimiter.getLine(lineView)) {
    if (linesDiscardDelimiter._last) {
      ASSERT_EQ(lineView, lastLine);
    }
  }
}

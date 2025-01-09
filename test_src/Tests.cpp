/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ClientOptions.h"
#include "Compression.h"
#include "FileLines.h"
#include "IOUtility.h"
#include "Logger.h"
#include "StringLines.h"
#include "TestEnvironment.h"
#include "Utility.h"

// ./testbin --gtest_filter=GetFileLineTest*
// gdb --args testbin --gtest_filter=GetFileLineTest*
// gdb --args testbin --gtest_filter=GetStringLineTest*

struct CompressionTest : testing::Test {
  void testCompressionDecompression(std::string& input) {
    // must be a copy
    std::string original = input;
    compression::compress(TestEnvironment::_buffer, input);
    std::size_t compressedSz = input.size();
    compression::uncompress(TestEnvironment::_buffer, input, original.size());
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    static auto& printOnce [[maybe_unused]] = logger
      << "\n\tinput.size()=" << input.size()
      << " compressedSize=" << compressedSz << " restored to original:"
      << std::boolalpha << (original == input) << '\n' << '\n';
    ASSERT_EQ(original, input);
  }
};

TEST_F(CompressionTest, 1_SOURCE) {
  // must be a copy
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTest, 1_OUTPUTD) {
  // must be a copy
  std::string input = TestEnvironment::_outputD;
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
  unsigned reserved = 0;
  unsigned uncomprSz = 123456;
  COMPRESSORS compressor = COMPRESSORS::LZ4;
  DIAGNOSTICS diagnostics = DIAGNOSTICS::ENABLED;
  HEADER header{HEADERTYPE::SESSION, 0, uncomprSz, compressor, diagnostics, STATUS::NONE, 0};
  serialize(header, buffer);
  ASSERT_TRUE(deserialize(header, buffer));
  std::size_t reservedResult = extractSalt(header);
  ASSERT_EQ(reservedResult, reserved);
  std::size_t uncomprSzResult = extractUncompressedSize(header);
  ASSERT_EQ(uncomprSz, uncomprSzResult);
  COMPRESSORS compressorResult = extractCompressor(header);
  ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
  DIAGNOSTICS diagnosticsResult = extractDiagnostics(header);
  ASSERT_EQ(diagnostics, diagnosticsResult);
  compressor = COMPRESSORS::NONE;
  header = {HEADERTYPE::SESSION, reserved, uncomprSz, compressor, diagnostics, STATUS::NONE, 0};
  serialize(header, buffer);
  ASSERT_TRUE(deserialize(header, buffer));
  compressorResult = extractCompressor(header);
  ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
}

TEST(GetFileLineTest, 1) {
  // get last line in the file using std::getline
  std::string lastLine;
  ASSERT_TRUE(utility::getLastLine(ClientOptions::_sourceName, lastLine));
  if (utility::fileEndsWithEOL(ClientOptions::_sourceName))
    lastLine.push_back('\n');

  // use FileLines::getLine
  std::string_view lineView;
  FileLines linesV(ClientOptions::_sourceName, '\n', true);
  while (linesV.getLine(lineView)) {
    if (linesV._last) {
      ASSERT_EQ(lineView, lastLine);
    }
  }

  std::string lineStr;
  FileLines linesS(ClientOptions::_sourceName, '\n', true);
  while (linesS.getLine(lineStr)) {
    if (linesS._last) {
      ASSERT_EQ(lineStr, lastLine);
    }
  }
  // discard delimiter:
  // keepDelimiter == false by default
  FileLines linesDiscardDelimiter(ClientOptions::_sourceName);
  lastLine.pop_back();
  while (linesDiscardDelimiter.getLine(lineView)) {
    if (linesDiscardDelimiter._last) {
      ASSERT_EQ(lineView, lastLine);
    }
  }
}

TEST(GetStringLineTest, 1) {
  // get last line in the file using std::getline
  std::string lastLine;
  ASSERT_TRUE(utility::getLastLine(ClientOptions::_sourceName, lastLine));
  if (utility::fileEndsWithEOL(ClientOptions::_sourceName))
    lastLine.push_back('\n');
  std::string_view lastLineCmp = lastLine;

  // use StringLines::getLine
  std::string_view line;

  StringLines linesKeepDelimeter(TestEnvironment::_source, '\n', true);
  while (linesKeepDelimeter.getLine(line)) {
    if (linesKeepDelimeter._last) {
      ASSERT_EQ(line, lastLineCmp);
    }
  }

  StringLines lines(TestEnvironment::_source);
  lastLineCmp.remove_suffix(1);
  while (lines.getLine(line)) {
    if (lines._last) {
      ASSERT_EQ(line, lastLineCmp);
    }
  }
}

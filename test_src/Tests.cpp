/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <snappy.h>

#include <boost/regex.hpp>

#include "ClientOptions.h"
#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "FileLines.h"
#include "IOUtility.h"
#include "StringLines.h"
#include "TestEnvironment.h"

// ./testbin --gtest_filter=GetFileLineTest*
// gdb --args testbin --gtest_filter=GetFileLineTest*
// gdb --args testbin --gtest_filter=GetStringLineTest*
// for i in {1..10}; do ./testbin --gtest_filter=regex*;done

struct CompressionTestLZ4 : testing::Test {
  void testCompressionDecompression(std::string& input) {
    // must be a copy
    std::string original = input;
    compressionLZ4::compress(TestEnvironment::_buffer, input);
    std::size_t compressedSz = input.size();
    compressionLZ4::uncompress(TestEnvironment::_buffer, input, original.size());
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    logger << "\n\tinput.size()=" << input.size()
	   << " compressedSize=" << compressedSz << " restored to original:"
	   << std::boolalpha << (original == input) << '\n' << '\n';
    ASSERT_EQ(original, input);
  }
};

TEST_F(CompressionTestLZ4, 1_SOURCE) {
  // must be a copy
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestLZ4, 1_OUTPUTD) {
  // must be a copy
  std::string input = TestEnvironment::_outputD;
  testCompressionDecompression(TestEnvironment::_outputD);
}

struct CompressionTestSnappy : testing::Test {
  void testCompressionDecompression(std::string& input) {
    // must be a copy
    std::string original = input;
    compressionSnappy::compress(TestEnvironment::_buffer, input);
    std::size_t compressedSz = input.size();
    compressionSnappy::uncompress(TestEnvironment::_buffer, input);
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    logger << "\n\tinput.size()=" << input.size()
	   << " compressedSize=" << compressedSz << " restored to original:"
	   << std::boolalpha << (original == input) << '\n' << '\n';
    ASSERT_EQ(original, input);
  }
};

TEST_F(CompressionTestSnappy, 1_SOURCE) {
  // must be a copy
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestSnappy, 1_OUTPUTD) {
  // must be a copy
  std::string input = TestEnvironment::_outputD;
  testCompressionDecompression(TestEnvironment::_outputD);
}

struct CompressionTestZSTD : testing::Test {
  void testCompressionDecompression(std::string& input) {
    // must be a copy
    std::string original = input;
    compressionZSTD::compress(TestEnvironment::_buffer, input);
    std::size_t compressedSz = input.size();
    compressionZSTD::uncompress(TestEnvironment::_buffer, input);
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    logger << "\n\tinput.size()=" << input.size()
	   << " compressedSize=" << compressedSz << " restored to original:"
	   << std::boolalpha << (original == input) << '\n' << '\n';
    ASSERT_EQ(original, input);
  }
};

TEST_F(CompressionTestZSTD, 1_SOURCE) {
  // must be a copy
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestZSTD, 1_OUTPUTD) {
  // must be a copy
  std::string input = TestEnvironment::_outputD;
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
  CRYPTO crypto = CRYPTO::ENCRYPT;
  COMPRESSORS compressor = COMPRESSORS::LZ4;
  DIAGNOSTICS diagnostics = DIAGNOSTICS::ENABLED;
  HEADER header{HEADERTYPE::SESSION, 0, uncomprSz, crypto, compressor, diagnostics, STATUS::NONE, 0};
  serialize(header, buffer);
  ASSERT_TRUE(deserialize(header, buffer));
  std::size_t reservedResult = extractReservedSz(header);
  ASSERT_EQ(reservedResult, reserved);
  std::size_t uncomprSzResult = extractUncompressedSize(header);
  ASSERT_EQ(uncomprSz, uncomprSzResult);
  COMPRESSORS compressorResult = extractCompressor(header);
  ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
  DIAGNOSTICS diagnosticsResult = extractDiagnostics(header);
  ASSERT_EQ(diagnostics, diagnosticsResult);
  compressor = COMPRESSORS::NONE;
  header = {HEADERTYPE::SESSION, reserved, uncomprSz, crypto, compressor, diagnostics, STATUS::NONE, 0};
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

TEST(ClearPreservesCapacity, 1) {
  std::string copy = TestEnvironment::_source;
  std::size_t orgCapacity = copy.capacity();
  copy.clear();
  std::size_t finalCapacity = copy.capacity();
  ASSERT_EQ(orgCapacity, finalCapacity);
}

// Neither boost nor std regex do not work with string_view
TEST(regex, 1) {
  std::string request("http://bid.simpli.fi/ck_bid?size=728x90&user_agent\
=Mozilla/5.0%20(compatible;%20MSIE%209.0;%20Windows%20NT%206.1;%20WOW64;%20Trident/5.0)\
&kw=discountcomputer+gliclazide&ip_address=67.87.131.40&site_id=95");
  static const boost::regex pattern("size=\\d+x\\d+&");
  boost::smatch match;
  ASSERT_TRUE(boost::regex_search(request, match, pattern));
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "\t\tmatch:" << match.str() << '\n';
}

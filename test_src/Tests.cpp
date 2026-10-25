/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include <boost/regex.hpp>

#include "ClientOptions.h"
#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "FileLines.h"
#include "FileLinesRange.h"
#include "IOUtility.h"
#include "StringLines.h"
#include "StringLinesRange.h"
#include "TestEnvironment.h"

// ./testbin --gtest_filter=FileLineTest*
// gdb --args testbin --gtest_filter=FileLineTest*
// gdb --args testbin --gtest_filter=StringLineTest*
// for i in {1..10}; do ./testbin --gtest_filter=regex*;done
// for i in {1..10}; do ./testbin --gtest_filter=StringLinesRangeTest*;done
// for i in {1..10}; do ./testbin --gtest_filter=FileLinesRangeTest*;done

struct CompressionTestLZ4 : testing::Test {
  void testCompressionDecompression(std::string& input) {
    std::string original = input;
    compressionLZ4::compress(TestEnvironment::_buffer, input);
    std::size_t compressedSz = input.size();
    compressionLZ4::uncompress(TestEnvironment::_buffer, input);
    Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
    logger << "\n\tinput.size()=" << input.size()
	   << " compressedSize=" << compressedSz << " restored to original:"
	   << std::boolalpha << (original == input) << '\n' << '\n';
    ASSERT_EQ(original, input);
  }
};

TEST_F(CompressionTestLZ4, 1_SOURCE) {
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestLZ4, 1_OUTPUTD) {
  std::string input = TestEnvironment::_outputD;
  testCompressionDecompression(TestEnvironment::_outputD);
}

struct CompressionTestSnappy : testing::Test {
  void testCompressionDecompression(std::string& input) {
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
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestSnappy, 1_OUTPUTD) {
  std::string input = TestEnvironment::_outputD;
  testCompressionDecompression(TestEnvironment::_outputD);
}

struct CompressionTestZSTD : testing::Test {
  void testCompressionDecompression(std::string& input) {
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
  std::string input = TestEnvironment::_source;
  testCompressionDecompression(input);
}

TEST_F(CompressionTestZSTD, 1_OUTPUTD) {
  std::string input = TestEnvironment::_outputD;
  testCompressionDecompression(TestEnvironment::_outputD);
}

TEST(SplitTest, NoKeepDelim) {
  std::vector<std::string_view> lines;
  utility::splitRanges(TestEnvironment::_source, lines);
  ASSERT_EQ(lines.size(), 10001);
  for (const auto& line : lines)
    ASSERT_FALSE(line.ends_with('\n'));
}

TEST(SplitTest, KeepDelim) {
  std::vector<std::string_view> lines;
  std::string contents(utility::readFile(ClientOptions::_sourceName));
  utility::splitRanges(contents, lines, '\n', 1);
  ASSERT_EQ(lines.size(), 10001);
  ASSERT_TRUE(contents.starts_with(lines[0]));
  ASSERT_TRUE(contents.ends_with(lines[9999]));
  ASSERT_FALSE(contents.find(lines[5432]) == std::string::npos);
}

TEST(SplitTest, Chars) {
  std::vector<std::string_view> chars;
  std::string source("A|B|C|D|E");
  utility::splitRanges(source, chars, '|', 1);
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

TEST(SplitRanges, 1) {
  std::vector<std::string_view> tokens;
  std::string input("AB\nCD\nEF\nGI\n\n");
  utility::splitRanges(input, tokens, '\n', 1);
  for (std::size_t i : {1, 2, 3, 4})
    ASSERT_TRUE(tokens[i].ends_with('\n'));
  ASSERT_TRUE(tokens.back().empty());
  std::string restored;
  for (auto token : tokens)
    restored.append(token);
  ASSERT_TRUE(restored == input);
  utility::splitRanges(input, tokens, '\n');
  for (std::size_t i = 0; i < tokens.size(); ++i)
    ASSERT_FALSE(tokens[i].ends_with('\n'));
  ASSERT_TRUE(tokens.back().empty());
}

TEST(ToCharsTest, Integral) {
  int value = 7;
  auto converted(ioutility::toCharsBoost(value));
  ASSERT_TRUE(converted[0]== '7');
}

TEST(FromCharsTest, Integral0) {
  std::string_view view = "0000";
  int value = 7;
  ioutility::fromCharsBoost(view, value);
  ASSERT_EQ(value, 0);
}

TEST(FromCharsTest, Integral) {
  std::string_view view = "123456789";
  int value = {};
  ioutility::fromCharsBoost(view, value);
  ASSERT_EQ(value, 123456789);
}

TEST(FromCharsTest, FloatingPoint) {
  std::string_view view = "1234567.89";
  double value = {};
  ioutility::fromChars(view, value);
  ASSERT_EQ(value, 1234567.89);
}

TEST(HeaderTest, 1) {
  unsigned field1Sz = 0;
  unsigned field2Sz = 123456;
  COMPRESSORS compressor = COMPRESSORS::LZ4;
  DIAGNOSTICS diagnostics = DIAGNOSTICS::ENABLED;
  HEADER header{HEADERTYPE::SESSION, 0, field2Sz, compressor, diagnostics, STATUS::NONE, 0, 0 };
  auto buffer = serialize(header);
  ASSERT_TRUE(deserialize(header, buffer.data()));
  std::size_t field1SzResult = extractField1Size(header);
  ASSERT_EQ(field1SzResult, field1Sz);
  std::size_t field2SzResult = extractField2Size(header);
  ASSERT_EQ(field2Sz, field2SzResult);
  COMPRESSORS compressorResult = extractCompressor(header);
  ASSERT_EQ(COMPRESSORS::LZ4, compressorResult);
  DIAGNOSTICS diagnosticsResult = extractDiagnostics(header);
  ASSERT_EQ(diagnostics, diagnosticsResult);
  compressor = COMPRESSORS::NONE;
  header = {HEADERTYPE::SESSION, field1Sz, field2Sz, compressor, diagnostics, STATUS::NONE, 0, 0 };
  auto buffer2 = serialize(header);
  ASSERT_TRUE(deserialize(header, buffer2.data()));
  compressorResult = extractCompressor(header);
  ASSERT_EQ(compressorResult, COMPRESSORS::NONE);
}

TEST(FileLinesTest, 1) {
  std::string sourceCopy;
  FileLines lines(ClientOptions::_sourceName, '\n', 1);
  std::string line;
  while (lines.getLine(line))
    sourceCopy += line;
  ASSERT_EQ(sourceCopy, TestEnvironment::_source);
}

TEST(FileLinesRangeTest, 1) {
  std::string sourceCopy;
  FileLinesRange lines('\n', 1);
  std::string_view line;
  while (lines.getLine(line))
    sourceCopy.append(line);
  ASSERT_TRUE(lines._last);
  ASSERT_EQ(sourceCopy, lines._source);
}

TEST(StringLineTest, 1) {
  std::string sourceCopy;
  StringLines lines(TestEnvironment::_source, '\n', 1);
  std::string_view line;
  while (lines.getLine(line))
    sourceCopy += line;
  ASSERT_EQ(sourceCopy, TestEnvironment::_source);
}

TEST(StringLinesRangeTest, 1) {
  std::string sourceCopy;
  StringLinesRange lines(TestEnvironment::_source, '\n', 1);
  std::string_view line;
  while (lines.getLine(line))
    sourceCopy.append(line);
  ASSERT_TRUE(lines._last);
  ASSERT_EQ(sourceCopy, lines._source);
}

TEST(FileLinesRangeTest, 2) {
  FileLinesRange linesR('\n', 1);
  ASSERT_TRUE(FileLinesRange::_source.ends_with('\n'));
  ASSERT_FALSE(FileLinesRange::_source.ends_with("\n\n"));
  ASSERT_EQ(linesR._lines.size(), 10001);
  ASSERT_TRUE(linesR._lines[10000].empty());
 ASSERT_FALSE(linesR._lines[9999].ends_with("\n\n"));
 std::string testEntry9999;
 ASSERT_TRUE(linesR.getLine(testEntry9999, 9999));
 ASSERT_TRUE(testEntry9999.starts_with("[9999]"));
 std::string testEntry7654;
 ASSERT_TRUE(linesR.getLine(testEntry7654, 7654));
 ASSERT_TRUE(testEntry7654.starts_with("[7654]"));
}

TEST(FileLinesRangeTest, 3) {
  FileLinesRange linesR('\n', 1);
  std::vector<std::string_view> reversedLinesWithId;
  std::vector<std::string_view> reversed(linesR._linesWithId);
  std::string batch;
  std::reverse(reversed.begin(), reversed.end());
  ASSERT_EQ(linesR._linesWithId.size(), reversed.size());
  ASSERT_EQ(linesR._linesWithId.front(), reversed.back());
  for (std::size_t index = 0; index < reversed.size(); ++index)
    ASSERT_EQ(linesR._linesWithId[index], reversed[reversed.size()- 1 - index]);
  while (!reversed.empty()) {
    batch.append(reversed.back());
    reversed.pop_back();
  }
  ASSERT_TRUE(reversed.empty());
  ASSERT_EQ(batch, linesR._sourceWithIds);
}

TEST(ClearPreservesCapacity, 1) {
  std::string copy = TestEnvironment::_source;
  std::size_t orgCapacity = copy.capacity();
  copy.clear();
  std::size_t finalCapacity = copy.capacity();
  ASSERT_EQ(orgCapacity, finalCapacity);
}

// Neither boost nor std regex work with string_view
TEST(regex, 1) {
  std::string request("http://bid.simpli.fi/ck_bid?size=728x90&user_agent\
=Mozilla/5.0%20(compatible;%20MSIE%209.0;%20Windows%20NT%206.1;%20WOW64;%20Trident/5.0)\
&kw=discountcomputer+gliclazide&ip_address=67.87.131.40&site_id=95");
  const boost::regex pattern("size=\\d+x\\d+&");
  boost::smatch match;
  ASSERT_TRUE(boost::regex_search(request, match, pattern));
  Logger logger(LOG_LEVEL::ALWAYS, std::clog, false);
  logger << "\t\tmatch:" << match.str() << '\n';
}

TEST(stringToVector, 1) {
  std::string string1("Y+\0012345\0678987654\0321X3\0");
  std::vector<unsigned char> vector1(string1.cbegin(), string1.cend());
  std::string string2(vector1.cbegin(), vector1.cend());
  std::string_view stringSV(string2);
  std::vector<unsigned char> vector2(string2.cbegin(), string2.cend());
  std::vector<unsigned char> vector3(stringSV.cbegin(), stringSV.cend());
  ASSERT_EQ(vector1, vector2);
  ASSERT_EQ(vector2, vector3);
  std::span<unsigned char> span2(vector2);
  std::span<unsigned char> span3(vector3);
  std::vector<unsigned char> vector4(span2.cbegin(), span2.cend());
  std::vector<unsigned char> vector5(span3.cbegin(), span3.cend());
  ASSERT_EQ(vector4, vector5);
}

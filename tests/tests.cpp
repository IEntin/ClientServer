#include "Compression.h"
#include "Utility.h"
#include <gtest/gtest.h>
#include <cstring>
#include <fstream>
#include <iostream>

std::string readContent(const char* sourceName) {
  std::ifstream ifs(sourceName);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << sourceName << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

bool testCompressionDecompression1(std::string_view input) {
  std::string_view compressedView = Compression::compress(input);
  if (compressedView.empty())
    return false;
  // save in a string before buffer is reused in uncompress
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
  const std::string content = readContent("client_bin/requests.log");
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression1(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(CompressionTest, CompressionTest2) {
  bool compressionTestResult = true;
  const std::string content = readContent("client_bin/output.txt");
  for (int i = 0; i < 10; ++i)
    compressionTestResult = compressionTestResult && testCompressionDecompression2(content);
  ASSERT_TRUE(compressionTestResult && !content.empty());
}

TEST(SplitTest, SplitTest1) {
  const std::string content = readContent("client_bin/requests.log");
  std::vector<std::string_view> lines;
  utility::split(content, lines);
  ASSERT_EQ(lines.size(), 10000);
}

TEST(FromCharsTest, FromCharsTest1) {
  const std::string_view view = "123456789";
  int value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 123456789);
}

TEST(FromCharsTest, FromCharsTest2) {
  const std::string_view view = "1234567.89";
  double value = {};
  bool ok = utility::fromChars(view, value);
  ASSERT_TRUE(ok);
  ASSERT_EQ(value, 1234567.89);
}

int main(int argc, char **argv) {
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

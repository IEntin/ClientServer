#include "Chronometer.h"
#include "Compression.h"
#include <gtest/gtest.h>
#include <fstream>
#include <iostream>

namespace {

bool compressionTestResult1 = true;
bool compressionTestResult2 = true;

} // end of anonimous namespace

std::string readContent() {
  std::ifstream ifs("client_bin/requests.log");
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
  static auto& printOnce [[maybe_unused]] = std::clog << "   input.size()=" << input.size() << " compressedView.size()="
	    << compressedView.size() << " restored to original:" << std::boolalpha
	    << (input == uncompressedView) << std::endl;
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
  static auto& printOnce [[maybe_unused]] = std::clog << "   input.size()=" << input.size() << " compressedView.size()="
	    << compressedView.size() << " restored to original:" << std::boolalpha
	    << (input == uncompressedView) << std::endl;
  return input == uncompressedView;
}

TEST(CompressionTest, CompressionTest1) {
  ASSERT_TRUE(compressionTestResult1);
}

TEST(CompressionTest, CompressionTest2) {
  ASSERT_TRUE(compressionTestResult2);
}

int main(int argc, char **argv) {
  const std::string  original = readContent();
  Chronometer chronometer(true, __FILE__, __LINE__, __func__);
  for (int i = 0; i < 10; ++i) {
    compressionTestResult1 = compressionTestResult1 && testCompressionDecompression1(original);
    compressionTestResult2 = compressionTestResult2 && testCompressionDecompression2(original);
  }
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

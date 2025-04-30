/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"

#include <atomic>
#include <filesystem>
#include <fstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include "CompressionLZ4.h"
#include "CompressionSnappy.h"
#include "CompressionZSTD.h"
#include "CryptoPlPl.h"

namespace utility {

std::string serverTerminal;
std::string clientTerminal;
std::string testbinTerminal;

std::u8string generateRawUUID() {
  boost::uuids::random_generator_mt19937 gen;
  boost::uuids::uuid uuid = gen();
  return { uuid.begin(), uuid.end() };
}

std::size_t getUniqueId() {
  static std::atomic<size_t> uniqueInteger;
  return uniqueInteger.fetch_add(1);
}

void readFile(std::string_view fileName, std::string& buffer) {
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  std::uintmax_t size = std::filesystem::file_size(fileName);
  buffer.resize(size);
  stream.read(buffer.data(), buffer.size());
}

// used in tests
bool getLastLine(std::string_view fileName, std::string& lastLine) {
  char ch;
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  stream.seekg(-2, std::ios_base::end);
  stream.get(ch);
  while (ch != '\n') {
    stream.seekg(-2, std::ios_base::cur);
    stream.get(ch);
  }
  std::getline(stream, lastLine);
  return true;
}

// used in tests
bool fileEndsWithEOL(std::string_view fileName) {
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  stream.seekg(-1, std::ios::end);
  char ch;
  stream.get(ch);
  return ch == '\n';
}

std::string_view compressEncrypt(std::string& buffer,
				 const HEADER& header,
				 CryptoWeakPtr weak,
				 std::string& data,
				 int compressionLevel) {
  if (isCompressed(header)) {
    COMPRESSORS compressor = extractCompressor(header);
    if (compressor == COMPRESSORS::LZ4)
      compressionLZ4::compress(buffer, data);
    else if (compressor == COMPRESSORS::SNAPPY)
      compressionSnappy::compress(buffer, data);
    else if (compressor == COMPRESSORS::ZSTD)
      compressionZSTD::compress(buffer, data, compressionLevel);
  }
  if (doEncrypt(header)) {
    if (auto crypto = weak.lock(); crypto)
      return crypto->encrypt(buffer, header, data);
  }
  else {
    char headerBuffer[HEADER_SIZE] = {};
    serialize(header, headerBuffer);
    data.insert(0, headerBuffer, HEADER_SIZE);
  }
  return data;
}

void decryptDecompress(std::string& buffer,
		       HEADER& header,
		       CryptoWeakPtr weak,
		       std::string& data) {
  if (auto crypto = weak.lock();crypto) {
    crypto->decrypt(buffer, data);
    deserialize(header, data.data());
    data.erase(0, HEADER_SIZE);
    if (isCompressed(header)) {
      COMPRESSORS compressor = extractCompressor(header);
      if (compressor == COMPRESSORS::LZ4) {
	std::size_t uncomprSize = extractUncompressedSize(header);
	compressionLZ4::uncompress(buffer, data, uncomprSize);
      }
      else if (compressor == COMPRESSORS::SNAPPY)
	compressionSnappy::uncompress(buffer, data);
      else if (compressor == COMPRESSORS::ZSTD)
	compressionZSTD::uncompress(buffer, data);
    }
  }
}

void setServerTerminal(std::string_view terminal) {
  serverTerminal = terminal;
}

void setClientTerminal(std::string_view terminal) {
  clientTerminal = terminal;
}

void setTestbinTerminal(std::string_view terminal) {
  testbinTerminal = terminal;
}

bool isServerTerminal() {
  const std::string currentTerminal(getenv("GNOME_TERMINAL_SCREEN"));
  return currentTerminal == serverTerminal;
}

bool isClientTerminal() {
  const std::string currentTerminal(getenv("GNOME_TERMINAL_SCREEN"));
  return currentTerminal == clientTerminal;
}

bool isTestbinTerminal() {
  const std::string currentTerminal(getenv("GNOME_TERMINAL_SCREEN"));
  return currentTerminal == testbinTerminal;
}

void removeAccess() {
  std::string().swap(serverTerminal);
  std::string().swap(clientTerminal);
}

} // end of namespace utility

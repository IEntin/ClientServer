/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"

#include <atomic>
#include <filesystem>
#include <fstream>
#include <regex>

#include "Compression.h"
#include "IOUtility.h"
#include "Logger.h"

namespace utility {

std::string serverTerminal;
std::string clientTerminal;
std::string testbinTerminal;

CloseFileDescriptor::CloseFileDescriptor(int& fd) : _fd(fd) {}

CloseFileDescriptor::~CloseFileDescriptor() {
  if (_fd != -1 && close(_fd) == -1)
    LogError << strerror(errno) << '\n';
  _fd = -1;
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
  char ch = 'x';
  std::ifstream stream;
  stream.exceptions(std::ifstream::failbit | std::ifstream::badbit);
  stream.open(fileName.data(), std::ios::binary);
  stream.seekg(-1, std::ios::end);
  stream.get(ch);
  return ch == '\n';
}

std::string createErrorString(std::errc ec,
			      const boost::source_location& location) {
  std::string msg(std::make_error_code(ec).message());
  msg.append(1, ':').append(location.file_name()).append(1, ':');
  ioutility::toChars(location.line(), msg);
  return msg.append(1, ' ').append(location.function_name());
}

std::string createErrorString(const boost::source_location& location) {
  std::string msg(strerror(errno));
  msg.append(1, ':').append(location.file_name()).append(1, ':');
  ioutility::toChars(location.line(), msg);
  return msg.append(1, ' ').append(location.function_name());
}

void compressEncrypt(std::string& buffer,
		     bool encrypt,
		     const HEADER& header,
		     CryptoWeakPtr weak,
		     std::string& data) {
  if (isCompressed(header))
    compression::compress(buffer, data);
  char headerBuffer[HEADER_SIZE] = {};
  serialize(header, headerBuffer);
  data.insert(0, headerBuffer, HEADER_SIZE);
  if (auto crypto = weak.lock();crypto)
    crypto->encrypt(buffer, encrypt, data);
}

void decryptDecompress(std::string& buffer,
		       HEADER& header,
		       CryptoWeakPtr weak,
		       std::string& data) {
  if (auto crypto = weak.lock();crypto) {
    crypto->decrypt(buffer, data);
    std::string_view headerView = std::string_view(data.data(), HEADER_SIZE);
    deserialize(header, headerView.data());
    data.erase(0, HEADER_SIZE);
    if (isCompressed(header)) {
      std::size_t uncomprSize = extractUncompressedSize(header);
      compression::uncompress(buffer, data, uncomprSize);
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

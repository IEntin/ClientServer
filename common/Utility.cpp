/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Header.h"
#include "Utility.h"

#include <atomic>
#include <filesystem>
#include <fstream>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>

namespace utility {

std::string serverTerminal;
std::string clientTerminal;
std::string testbinTerminal;

std::string generateRawUUID() {
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

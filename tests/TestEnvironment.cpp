/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "TestEnvironment.h"
#include "ClientOptions.h"

volatile std::sig_atomic_t stopFlag = false;

std::string TestEnvironment::_source;
std::string TestEnvironment::_outputD;

void TestEnvironment::SetUp() {
  ClientOptions clientOptions;
  _source = readFile(clientOptions._sourceName);
  _outputD = readFile("data/outputD.txt");
}

std::string TestEnvironment::readFile(const std::string& name) {
  std::ifstream ifs(name, std::ifstream::in | std::ifstream::binary);
  if (!ifs) {
    std::cerr << __FILE__ << ':' << __LINE__ << ' ' << __func__ << ':'
	      << std::strerror(errno) << ' ' << name << std::endl;
    return "";
  }
  std::stringstream buffer;
  buffer << ifs.rdbuf();
  return buffer.str();
}

void TestEnvironment::TearDown() {}

#include "Client.h"
#include "ClientOptions.h"
#include <cassert>
#include <cstring>
#include <iostream>
#include <sstream>

Client::Client(const ClientOptions& options) : _options(options), _threadPool(1) {}

Client::~Client() {
  _threadPool.stop();
}

std::string Client::readFileContent(const std::string& name) {
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

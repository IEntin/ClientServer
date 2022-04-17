/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Utility.h"
#include <cstring>
#include <iostream>
#include <fstream>
#include <sstream>

namespace utility {

std::string readFile(const std::string& name) {
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

} // end of namespace utility

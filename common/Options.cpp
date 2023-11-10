/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"
#include "AppOptions.h"
#include <filesystem>

std::string Options::_fifoDirectoryName;
std::string Options::_acceptorName;

void Options::parse(AppOptions& appOptions) {
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
}

/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "DebugLog.h"

std::ofstream DebugLog::_file;

void DebugLog::setDebugLog([[maybe_unused]] APPTYPE type) {
#ifdef _DEBUG
  switch (type) {
  case APPTYPE::TESTS:
    _file.open("debugTests.txt", std::ios_base::out | std::ios::trunc);
    break;
  case APPTYPE::SERVER:
    _file.open("debugServer.txt", std::ios_base::out | std::ios::trunc);
    break;
  case APPTYPE::CLIENT:
    _file.open("debugClient.txt", std::ios_base::out | std::ios::trunc);
    break;
  default:
    break;
  }
#endif
}

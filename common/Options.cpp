/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"
#include "AppOptions.h"
#include "Logger.h"
#include <filesystem>

std::string Options::_fifoDirectoryName;
std::string Options::_acceptorName;
unsigned short Options::_tcpPort;
std::string Options::_tcpService;
int Options::_numberRepeatENXIO;
int Options::_ENXIOwait;
bool Options::_showKey;
bool Options::_timing;
bool Options::_setPipeSize;
size_t Options::_pipeSize;
int Options::_cryptoKeySize = 32;

void Options::parse(AppOptions& appOptions) {
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _tcpPort = appOptions.get("TcpPort", 49151);
  _tcpService = std::to_string(_tcpPort);
  // next 2 parameters may be decreased for better responsiveness
  // or increased to prevent deadlocking on slow machines.
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 50);
  _ENXIOwait = appOptions.get("ENXIOwai", 10);
  _cryptoKeySize = appOptions.get("EncryptionKeySize", 32);
  _showKey = appOptions.get("ShowKey", false);
  _timing = appOptions.get("Timing", false);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _pipeSize = appOptions.get("PipeSize", 1000000);
  Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

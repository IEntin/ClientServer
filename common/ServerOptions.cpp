/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "ServerOptions.h"

#include <filesystem>
#include <thread>

#include "AppOptions.h"
#include "Header.h"
#include "Logger.h"

bool ServerOptions::_parsed = false;
std::string ServerOptions::_adsFileName;
std::string ServerOptions::_fifoDirectoryName;
std::string ServerOptions::_acceptorName;
COMPRESSORS ServerOptions::_compressor;
bool ServerOptions::_encrypted;
bool ServerOptions::_showKey;
int ServerOptions::_numberWorkThreads;
int ServerOptions::_maxTcpSessions;
int ServerOptions::_maxFifoSessions;
int ServerOptions::_maxTotalSessions;
unsigned short ServerOptions::_tcpPort;
int ServerOptions::_tcpTimeout;
bool ServerOptions::_sortInput;
bool ServerOptions::_timing;
int ServerOptions::_numberRepeatENXIO;
bool ServerOptions::_setPipeSize;
std::size_t ServerOptions::_pipeSize;
bool ServerOptions::_printHeader;

void ServerOptions::parse(std::string_view jsonName) {
  _parsed = true;
  AppOptions appOptions(jsonName);
  _adsFileName = appOptions.get("AdsFileName", std::string("data/ads.txt"));
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorName = _fifoDirectoryName + '/' + appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _compressor = translateCompressorString(appOptions.get("Compression", std::string("LZ4")));
  _encrypted = appOptions.get("Crypto", true);
  _showKey = appOptions.get("ShowKey", false);
  int numberWorkThreadsCfg = appOptions.get("NumberWorkThreads", 0);
  _numberWorkThreads = numberWorkThreadsCfg ? numberWorkThreadsCfg : std::thread::hardware_concurrency();
  _maxTcpSessions = appOptions.get("MaxTcpSessions", 2);
  _maxFifoSessions = appOptions.get("MaxFifoSessions", 2);
  _maxTotalSessions = appOptions.get("MaxTotalSessions", 2);
  _tcpPort = appOptions.get("TcpPort", 49151);
  _tcpTimeout = appOptions.get("TcpTimeout", 3000);
  _sortInput = appOptions.get("SortInput", true);
  _timing = appOptions.get("Timing", false);
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 200);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _pipeSize = appOptions.get("PipeSize", 1000000);
  _printHeader = appOptions.get("PrintHeader", false);
  Logger::translateLogThreshold(appOptions.get("LogThreshold", std::string("ERROR")));
}

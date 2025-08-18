/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"

#include <filesystem>

#include "AppOptions.h"

std::string Options::_fifoDirectoryName;
std::string Options::_acceptorBaseName;
std::string Options::_acceptorName;
int Options::_numberRepeatENXIO;
bool Options::_setPipeSize;
std::size_t Options::_pipeSize;
std::string Options::_serverAddress;
unsigned short Options::_tcpPort;
COMPRESSORS Options::_compressor;
bool Options::_printHeader;

void Options::parse(std::string_view jsonName) {
  AppOptions appOptions(jsonName);
  _fifoDirectoryName = appOptions.get("FifoDirectoryName", std::filesystem::current_path().string());
  _acceptorBaseName = appOptions.get("AcceptorBaseName", std::string("acceptor"));
  _acceptorName = _fifoDirectoryName + '/' + _acceptorBaseName;
  _numberRepeatENXIO = appOptions.get("NumberRepeatENXIO", 200);
  _setPipeSize = appOptions.get("SetPipeSize", true);
  _pipeSize = appOptions.get("PipeSize", 1000000);
  _serverAddress = appOptions.get("ServerAddress", std::string("127.0.0.1"));
  _tcpPort = appOptions.get("TcpPort", 49151);
  _compressor = translateCompressorString(appOptions.get("Compression", std::string("LZ4")));
  _printHeader = appOptions.get("PrintHeader", false);
}

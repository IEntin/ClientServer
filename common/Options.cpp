/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"

#include <filesystem>

#include "BoostJsonParser.h"

boost::static_string<100> Options::_fifoDirectoryName(std::filesystem::current_path().string());
boost::static_string<100> Options::_acceptorBaseName("acceptor");
boost::static_string<100> Options::_acceptorName(_fifoDirectoryName + '/' + _acceptorBaseName);
int Options::_numberRepeatENXIO(200);
bool Options::_setPipeSize(true);
std::size_t Options::_pipeSize(1000000);
boost::static_string<100> Options::_serverAddress("127.0.0.1");
unsigned short Options::_tcpPort(49151);
boost::json::value Options::_jv;

void Options::parse(std::string_view jsonName) {
  if (!jsonName.empty()) {
    parseJson(jsonName, _jv);
    _fifoDirectoryName = _jv.at("FifoDirectoryName").as_string();
    _acceptorBaseName = _jv.at("AcceptorBaseName").as_string();
    _acceptorName = _fifoDirectoryName + '/' + _acceptorBaseName;
    _numberRepeatENXIO = _jv.at("NumberRepeatENXIO").as_int64();
    _setPipeSize = _jv.at("SetPipeSize").as_bool();
    _pipeSize = _jv.at("PipeSize").as_int64();
    _serverAddress = _jv.at("ServerAddress").as_string();
    _tcpPort = _jv.at("TcpPort").as_int64();
  }
}

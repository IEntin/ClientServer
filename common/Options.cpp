/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"

#include <filesystem>

#include "BoostJsonParser.h"

CRYPTO Options::_encryptorType(translateCryptoString("CRYPTOSODIUM"));
std::string Options::_fifoDirectoryName(std::filesystem::current_path().string());
std::string Options::_acceptorBaseName("acceptor");
std::string Options::_acceptorName(_fifoDirectoryName + '/' + _acceptorBaseName);
int Options::_numberRepeatENXIO(200);
bool Options::_setPipeSize(true);
std::size_t Options::_pipeSize(1000000);
std::string Options::_serverAddress("127.0.0.1");
unsigned short Options::_tcpPort(49151);
boost::json::value Options::_jv;

void Options::parse(std::string_view jsonName) {
  if (!jsonName.empty()) {
    parseJson(jsonName, _jv);
    auto encryptorTypeString = _jv.at("EncryptorType").as_string();
    _encryptorType = translateCryptoString(encryptorTypeString);
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

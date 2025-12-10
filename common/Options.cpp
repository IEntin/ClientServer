/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"

#include <filesystem>

#include "BoostJsonParser.h"

CRYPTO Options::_encryptorType;
std::string Options::_fifoDirectoryName;
std::string Options::_acceptorBaseName;
std::string Options::_acceptorName;
int Options::_numberRepeatENXIO;
bool Options::_setPipeSize = true;
std::size_t Options::_pipeSize;
std::string Options::_serverAddress;
unsigned short Options::_tcpPort;

void Options::parse(std::string_view jsonName) {
  if (!jsonName.empty()) {
    boost::json::value jv;
    parseJson(jsonName, jv);
    auto encryptorTypeString = jv.at("EncryptorType").as_string();
    _encryptorType = translateCryptoString(encryptorTypeString);
    _fifoDirectoryName = jv.at("FifoDirectoryName").as_string();
    _acceptorBaseName = jv.at("AcceptorBaseName").as_string();
    _acceptorName = _fifoDirectoryName + '/' + _acceptorBaseName;
    _numberRepeatENXIO = jv.at("NumberRepeatENXIO").as_int64();
    _setPipeSize = jv.at("SetPipeSize").as_bool();
    _pipeSize = jv.at("PipeSize").as_int64();
    _serverAddress = jv.at("ServerAddress").as_string();
    _tcpPort = jv.at("TcpPort").as_int64();
  }
  else {
    _encryptorType = translateCryptoString("CRYPTOSODIUM");
    _fifoDirectoryName = std::filesystem::current_path().string();
    _acceptorBaseName = "acceptor";
    _acceptorName = _fifoDirectoryName + '/' + _acceptorBaseName;
    _numberRepeatENXIO = 200;
    _setPipeSize = true;
    _pipeSize = 1000000;
    _serverAddress = "127.0.0.1";
    _tcpPort = 49151;
  }
}

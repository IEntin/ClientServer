/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "Options.h"

#include <filesystem>

#include "BoostJsonParser.h"

CRYPTO Options::_singleEncryptor;
bool Options::_doubleEncryption;
boost::static_string<100> Options::_fifoDirectoryName(std::filesystem::current_path().string());
boost::static_string<100> Options::_acceptorBaseName;
boost::static_string<100> Options::_acceptorName(_fifoDirectoryName + '/' + _acceptorBaseName);
int Options::_numberRepeatENXIO;
bool Options::_setPipeSize;
std::size_t Options::_pipeSize;
boost::static_string<100> Options::_serverAddress;
unsigned short Options::_tcpPort;
bool Options::_printInitVector;

void Options::extractMatching(const boost::json::value& jv) {
  _singleEncryptor = translateCryptoString(jv.at("SingleEncryptor").as_string());
  _doubleEncryption = jv.at("DoubleEncryption").as_bool();
  _fifoDirectoryName = jv.at("FifoDirectoryName").as_string();
  _acceptorBaseName = jv.at("AcceptorBaseName").as_string();
  _acceptorName = _fifoDirectoryName + '/' + _acceptorBaseName;
  _numberRepeatENXIO = jv.at("NumberRepeatENXIO").as_int64();
  _setPipeSize = jv.at("SetPipeSize").as_bool();
  _pipeSize = jv.at("PipeSize").as_int64();
  _serverAddress = jv.at("ServerAddress").as_string();
  _tcpPort = jv.at("TcpPort").as_int64();
  _printInitVector = jv.at("PrintInitVector").as_bool();
}

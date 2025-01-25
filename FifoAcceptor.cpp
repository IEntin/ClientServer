/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"

#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "Logger.h"
#include "Options.h"
#include "Server.h"
#include "ServerOptions.h"

namespace fifo {

FifoAcceptor::FifoAcceptor(ServerPtr server) :
  _acceptorName(Options::_acceptorName),
  _server(server) {}

FifoAcceptor::~FifoAcceptor() {
  removeFifoFiles();
  Trace << '\n';
}

std::tuple<HEADERTYPE, std::string, CryptoPP::SecByteBlock, std::string>
FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, std::string(), CryptoPP::SecByteBlock(), std::string() };
  HEADER header;
  std::string msgHash;
  CryptoPP::SecByteBlock pubB;
  std::string rsaPubB;
  if (!Fifo::readMsg(_acceptorName, true, header, msgHash, pubB, rsaPubB))
    return { HEADERTYPE::ERROR, std::string(), CryptoPP::SecByteBlock(), rsaPubB };
  return { extractHeaderType(header), msgHash, pubB, rsaPubB };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, msgHash, pubB, signatureWithPubKey] = unblockAcceptor();
    if (_stopped)
      break;
    switch (type) {
    case HEADERTYPE::DH_INIT:
      if (auto server = _server.lock(); server)
	server->createFifoSession(msgHash, pubB, signatureWithPubKey);
      break;
    default:
      break;
    }
  }
}

bool FifoAcceptor::start() {
  // in case there was no proper shutdown.
  removeFifoFiles();
  if (mkfifo(_acceptorName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << strerror(errno) << '-' << _acceptorName << '\n';
    return false;
  }
  return true;
}

void FifoAcceptor::stop() {
  _stopped = true;
  Fifo::onExit(_acceptorName);
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(Options::_fifoDirectoryName))
    try {
      if (entry.is_fifo())
	std::filesystem::remove(entry);
    }
    catch (const std::exception& e) {
      Warn << e.what() << '\n';
    }
}

} // end of namespace fifo

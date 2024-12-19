/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"

#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "Logger.h"
#include "Server.h"
#include "ServerOptions.h"

namespace fifo {

FifoAcceptor::FifoAcceptor(ServerPtr server) :
  _acceptorName(ServerOptions::_acceptorName),
  _server(server) {}

FifoAcceptor::~FifoAcceptor() {
  removeFifoFiles();
  Trace << '\n';
}

std::tuple<HEADERTYPE, CryptoPP::SecByteBlock, std::vector<uint8_t>>
FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, CryptoPP::SecByteBlock(), std::vector<uint8_t>() };
  HEADER header;
  CryptoPP::SecByteBlock pubB;
  std::vector<uint8_t> rsaPubB;
  if (!Fifo::readMsg(_acceptorName, true, header, pubB, rsaPubB))
    return { HEADERTYPE::ERROR, CryptoPP::SecByteBlock(), rsaPubB };
  return { extractHeaderType(header), pubB, rsaPubB };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, pubB, signatureWithPubKey] = unblockAcceptor();
    if (_stopped)
      break;
    switch (type) {
    case HEADERTYPE::DH_INIT:
      if (auto server = _server.lock(); server)
	server->createFifoSession(pubB, signatureWithPubKey);
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
  for(auto const& entry : std::filesystem::directory_iterator(ServerOptions::_fifoDirectoryName))
    try {
      if (entry.is_fifo())
	std::filesystem::remove(entry);
    }
    catch (const std::exception& e) {
      Warn << e.what() << '\n';
    }
}

} // end of namespace fifo

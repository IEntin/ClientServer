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

std::tuple<HEADERTYPE, CryptoPP::SecByteBlock> FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, CryptoPP::SecByteBlock() };
  HEADER header;
  CryptoPP::SecByteBlock pubB;
  if (!Fifo::readMsg(_acceptorName, true, header, pubB))
    return { HEADERTYPE::ERROR, CryptoPP::SecByteBlock() };
  return { extractHeaderType(header), pubB };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, pubB] = unblockAcceptor();
    if (_stopped)
      break;
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      if (auto server = _server.lock(); server)
	server->createFifoSession(pubB);
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

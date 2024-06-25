/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"

#include <cstring>
#include <filesystem>
#include <sys/stat.h>

#include "Fifo.h"
#include "FifoSession.h"
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

std::tuple<HEADERTYPE, std::string> FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return { HEADERTYPE::ERROR, std::string() };
  HEADER header;
  std::string Bstring;
  if (!Fifo::readMsgBlock(_acceptorName, header, Bstring))
    return { HEADERTYPE::ERROR, std::string() };
  return { extractHeaderType(header), Bstring };
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto [type, Bstring] = unblockAcceptor();
    if (_stopped)
      break;
    auto session = std::make_shared<FifoSession>(Bstring);
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      if (auto server = _server.lock(); server)
	server->startSession(session);
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
    LogError << std::strerror(errno) << '-' << _acceptorName << '\n';
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
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo

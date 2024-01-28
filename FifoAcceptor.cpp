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

FifoAcceptor::FifoAcceptor(Server& server) :
  _acceptorName(ServerOptions::_acceptorName),
  _server(server) {}

FifoAcceptor::~FifoAcceptor() {
  removeFifoFiles();
  Trace << '\n';
}

HEADERTYPE FifoAcceptor::unblockAcceptor() {
  // blocks until the client opens writing end
  if (_stopped)
    return HEADERTYPE::ERROR;
  HEADER header;
  std::string body;
  if (!Fifo::readMsgBlock(_acceptorName, header, body))
    return HEADERTYPE::ERROR;
  return extractHeaderType(header);
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto session = std::make_shared<FifoSession>();
    auto type = unblockAcceptor();
    switch (type) {
    case HEADERTYPE::CREATE_SESSION:
      _server.startSession(session);
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

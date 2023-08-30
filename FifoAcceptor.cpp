/*
 *  Copyright (C) 2021 Ilya Entin
 */

#include "FifoAcceptor.h"
#include "Fifo.h"
#include "FifoSession.h"
#include "Server.h"
#include "ServerOptions.h"
#include <filesystem>
#include <sys/stat.h>

namespace fifo {

FifoAcceptor::FifoAcceptor(Server& server) :
  _server(server),
  _options(_server.getOptions()) {}

FifoAcceptor::~FifoAcceptor() {
  removeFifoFiles();
  Trace << '\n';
}

HEADERTYPE FifoAcceptor::unblockAcceptor() {
  static std::string emptyString;
  // blocks until the client opens writing end
  if (_stopped)
    return HEADERTYPE::ERROR;
  HEADER header;
  std::vector<char> body;
  if (!Fifo::readMsgBlock(_options._acceptorName, header, body))
    return HEADERTYPE::ERROR;
  return extractHeaderType(header);
}

void FifoAcceptor::run() {
  while (!_stopped) {
    auto session = std::make_shared<FifoSession>(_options);
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
  if (mkfifo(_options._acceptorName.data(), 0666) == -1 && errno != EEXIST) {
    LogError << std::strerror(errno) << '-' << _options._acceptorName << '\n';
    return false;
  }
  return true;
}

void FifoAcceptor::stop() {
  _stopped = true;
  Fifo::onExit(_options._acceptorName);
}

void FifoAcceptor::removeFifoFiles() {
  for(auto const& entry : std::filesystem::directory_iterator(_options._fifoDirectoryName))
    if (entry.is_fifo())
      std::filesystem::remove(entry);
}

} // end of namespace fifo
